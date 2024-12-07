#include "DICOMAppHelper.h"
#include "DICOMParser.h"
#include "raylib.h"
#include "rlImGui.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <iostream>
#include <stdint.h>

#define WIN_WIDTH 1366
#define WIN_HEIGHT 768

#define CUBE_SIZE 512

Camera3D camera = {.position = {90, 0, 0},
                   .target = {0, 0, 0},
                   .up = {0, 1, 0},
                   .fovy = 45,
                   .projection = CAMERA_PERSPECTIVE};
float brightness = 1.0f;

void drawDebugMenu();

void loadVolumeData(unsigned char *cube)
{
    //uint8_t ***volume = (uint8_t ***)(&cube[0]);
    DICOMAppHelper appHelper;
    DICOMParser parser;

    const std::string BaseFileName = ASSETS_PATH "dicom/PATIENT_DICOM/image_";
    int fileCount = 0;
    int maxFileCount = 124;

    while (fileCount < maxFileCount)
    {
        parser.ClearAllDICOMTagCallbacks();
        parser.OpenFile(BaseFileName + std::to_string(fileCount));
        appHelper.Clear();
        appHelper.RegisterCallbacks(&parser);
        appHelper.RegisterPixelDataCallback(&parser);

        parser.ReadHeader();

        void *imgData = nullptr;
        DICOMParser::VRTypes dataType;
        unsigned long imageDataLength = 0;

        appHelper.GetImageData(imgData, dataType, imageDataLength);
        auto numPixels = imageDataLength / 2;

        for (size_t i = 0; i < numPixels && i < CUBE_SIZE * CUBE_SIZE; ++i)
        {
            int16_t pixelVal = ((int16_t *)imgData)[i];
            if (pixelVal > 0)
            {
                uint8_t compressed = (uint8_t)((float(pixelVal) / float(INT16_MAX)) * UINT8_MAX);
                cube[fileCount * CUBE_SIZE * CUBE_SIZE + i] = compressed;
                cube[(fileCount + 1) * CUBE_SIZE * CUBE_SIZE + i] = compressed;
            }
        }
        fileCount += 2;
    }
}

int main()
{
    auto *cube = new unsigned char[CUBE_SIZE * CUBE_SIZE * CUBE_SIZE];
    for (int i = 0; i < CUBE_SIZE * CUBE_SIZE * CUBE_SIZE; ++i)
        cube[i] = 0;
    loadVolumeData(cube);

    InitWindow(WIN_WIDTH, WIN_HEIGHT, "DVR_GPU");
    rlImGuiSetup(true);

    const Vector2 resolution = {WIN_WIDTH, WIN_HEIGHT};
    const int iResolution[2] = {WIN_WIDTH, WIN_HEIGHT};

    // compute shader
    char *dvrComputeSource = LoadFileText(ASSETS_PATH "shaders/ray_cast.comp");
    auto dvrComputeShader = rlCompileShader(dvrComputeSource, RL_COMPUTE_SHADER);
    auto dvrComputeProgram = rlLoadComputeShaderProgram(dvrComputeShader);
    UnloadFileText(dvrComputeSource);

    // render shader (fragment)
    Shader dvrRenderShader = LoadShader(NULL, ASSETS_PATH "shaders/render.glsl");
    int resUniformLoc = GetShaderLocation(dvrRenderShader, "resolution");
    int brightUniformLoc = GetShaderLocation(dvrRenderShader, "brightness");

    // Load shader storage buffer object (SSBO), id returned
    constexpr auto bufferSize = WIN_WIDTH * WIN_HEIGHT * sizeof(int);
    auto ssboA = rlLoadShaderBuffer(bufferSize, NULL, RL_DYNAMIC_COPY);
    auto ssboB = rlLoadShaderBuffer(bufferSize, NULL, RL_DYNAMIC_COPY);

    // upload volume data
    constexpr auto volumeBufferSize = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE * sizeof(uint8_t);
    rlEnableShader(dvrComputeProgram);
    auto volumeDataSSBO = rlLoadShaderBuffer(volumeBufferSize, cube, RL_STATIC_READ);
    rlBindShaderBuffer(volumeDataSSBO, 4);
    int volumeSize = CUBE_SIZE;
    rlSetUniform(5, &volumeSize, RL_SHADER_UNIFORM_INT, 1);

    // Create a white texture of the size of the window to update
    // each pixel of the window using the fragment shader
    Image whiteImage = GenImageColor(WIN_WIDTH, WIN_HEIGHT, WHITE);
    Texture whiteTex = LoadTextureFromImage(whiteImage);
    UnloadImage(whiteImage);

    // Main game loop
    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_F))
            ToggleFullscreen();

        // ray cast
        rlEnableShader(dvrComputeProgram);
        rlBindShaderBuffer(ssboA, 1);
        rlBindShaderBuffer(ssboB, 2);
        rlSetUniform(3, iResolution, RL_SHADER_UNIFORM_IVEC2, 1);
        rlSetUniform(6, &camera, RL_SHADER_UNIFORM_FLOAT, 11);
        rlComputeShaderDispatch(static_cast<unsigned int>(ceil(WIN_WIDTH / 8.0)),
                                static_cast<unsigned int>(ceil(WIN_HEIGHT / 8.0)),
                                1);
        rlDisableShader();

        // swap SSBO's
        auto temp = ssboA;
        ssboA = ssboB;
        ssboB = temp;

        rlBindShaderBuffer(ssboA, 1);
        SetShaderValue(dvrRenderShader, resUniformLoc, &resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(dvrRenderShader, brightUniformLoc, &brightness, SHADER_UNIFORM_FLOAT);

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------

        BeginDrawing();

        ClearBackground(BLANK);

        BeginShaderMode(dvrRenderShader);
        DrawTexture(whiteTex, 0, 0, WHITE);
        EndShaderMode();

        DrawFPS(10, 10);
        drawDebugMenu();

        EndDrawing();
    }

    delete[] cube;

    // Unload shader buffers objects.
    rlUnloadShaderBuffer(ssboA);
    rlUnloadShaderBuffer(ssboB);
    rlUnloadShaderBuffer(volumeDataSSBO);

    // Unload compute shader programs
    rlUnloadShaderProgram(dvrComputeProgram);

    UnloadTexture(whiteTex);       // Unload white texture
    UnloadShader(dvrRenderShader); // Unload rendering fragment shader

    CloseWindow(); // Close window and OpenGL context

    return 0;
}

void drawDebugMenu()
{
    rlImGuiBegin();
    ImGui::Begin("Camera Controls");

    // Camera Position Controls
    ImGui::Text("Camera Position:");

    ImGui::PushID("CameraPosX");
    if (ImGui::DragFloat("X", &camera.position.x, 0.1f, -100.0f, 100.0f, "%.2f"))
    {
        // Optional: Logic when X position changes via slider
    }
    if (ImGui::InputFloat("Manual X", &camera.position.x, 0.0f, 0.0f, "%.2f"))
    {
        camera.position.x = std::clamp(camera.position.x, -100.0f, 100.0f);
    }
    ImGui::PopID();

    ImGui::PushID("CameraPosY");
    if (ImGui::DragFloat("Y", &camera.position.y, 0.1f, -100.0f, 100.0f, "%.2f"))
    {
        // Optional: Logic when Y position changes via slider
    }
    if (ImGui::InputFloat("Manual Y", &camera.position.y, 0.0f, 0.0f, "%.2f"))
    {
        camera.position.y = std::clamp(camera.position.y, -100.0f, 100.0f);
    }
    ImGui::PopID();

    ImGui::PushID("CameraPosZ");
    if (ImGui::DragFloat("Z", &camera.position.z, 0.1f, -100.0f, 100.0f, "%.2f"))
    {
        // Optional: Logic when Z position changes via slider
    }
    if (ImGui::InputFloat("Manual Z", &camera.position.z, 0.0f, 0.0f, "%.2f"))
    {
        camera.position.z = std::clamp(camera.position.z, -100.0f, 100.0f);
    }
    ImGui::PopID();

    // Camera Target Controls
    ImGui::Text("Camera Target:");

    ImGui::PushID("CameraTargetX");
    if (ImGui::SliderFloat("Target X", &camera.target.x, -180.0f, 180.0f, "%.2f"))
    {
        // Optional: Logic when X target changes via slider
    }
    if (ImGui::InputFloat("Manual X", &camera.target.x, 0.0f, 0.0f, "%.2f"))
    {
        camera.target.x = std::clamp(camera.target.x, -180.0f, 180.0f);
    }
    ImGui::PopID();

    ImGui::PushID("CameraTargetY");
    if (ImGui::SliderFloat("Target Y", &camera.target.y, -180.0f, 180.0f, "%.2f"))
    {
        // Optional: Logic when Y target changes via slider
    }
    if (ImGui::InputFloat("Manual Y", &camera.target.y, 0.0f, 0.0f, "%.2f"))
    {
        camera.target.y = std::clamp(camera.target.y, -180.0f, 180.0f);
    }
    ImGui::PopID();

    ImGui::PushID("CameraTargetZ");
    if (ImGui::SliderFloat("Target Z", &camera.target.z, -180.0f, 180.0f, "%.2f"))
    {
        // Optional: Logic when Z target changes via slider
    }
    if (ImGui::InputFloat("Manual Z", &camera.target.z, 0.0f, 0.0f, "%.2f"))
    {
        camera.target.z = std::clamp(camera.target.z, -180.0f, 180.0f);
    }
    ImGui::PopID();

    // Camera FOV Control
    ImGui::Text("Field of View:");
    ImGui::PushID("CameraFOV");
    if (ImGui::SliderFloat("FOV", &camera.fovy, 10.0f, 179.9f, "%.1f"))
    {
        // Optional: Logic when FOV changes via slider
    }
    if (ImGui::InputFloat("Manual FOV", &camera.fovy, 0.0f, 0.0f, "%.1f"))
    {
        camera.fovy = std::clamp(camera.fovy, 10.0f, 179.9f);
    }
    ImGui::PopID();

    // Image Brightness Control
    ImGui::Text("Brightness:");
    ImGui::PushID("Brightness");
    if (ImGui::SliderFloat("Brightness", &brightness, 0.0f, 10.0f, "%.1f"))
    {
        // Optional: Logic when FOV changes via slider
    }
    if (ImGui::InputFloat("Manual Brightness", &brightness, 0.0f, 0.0f, "%.1f"))
    {
        brightness = std::clamp(brightness, 0.0f, 10.0f);
    }
    ImGui::PopID();

    ImGui::End();
    rlImGuiEnd();
}