#include "DICOMAppHelper.h"
#include "DICOMParser.h"
#include "raylib.h"
#include "raymath.h"
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

Camera3D camera = {.position = {0, 0, -128},
                   .target = {0, 0, 0},
                   .up = {0, 1, 0},
                   .fovy = 45,
                   .projection = CAMERA_PERSPECTIVE};
float camRotateX = 90;
float camRotateY = 0;
float brightness = 1.0f;
bool applyMask = false;
float maskStrength[8] = {0, 0.15f, 0.1f, 0.6f, 1.0f, 0.7f, 0.7f, 0.5f};
int zoom = 128;

void drawDebugMenu();

void loadVolumeData(uint8_t *cube);

void loadVolumeMasks(uint8_t *masks);

int main()
{
    // 2 x 128 mb
    auto *cube = new uint8_t[CUBE_SIZE * CUBE_SIZE * CUBE_SIZE];
    auto *masks = new uint8_t[CUBE_SIZE * CUBE_SIZE * CUBE_SIZE];
    loadVolumeData(cube);
    loadVolumeMasks(masks);

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
    constexpr auto bufferSize = WIN_WIDTH * WIN_HEIGHT * sizeof(Vector4);
    auto ssboA = rlLoadShaderBuffer(bufferSize, NULL, RL_DYNAMIC_COPY);
    auto ssboB = rlLoadShaderBuffer(bufferSize, NULL, RL_DYNAMIC_COPY);

    // upload volume data
    constexpr auto volumeBufferSize = CUBE_SIZE * CUBE_SIZE * CUBE_SIZE * sizeof(uint8_t);
    rlEnableShader(dvrComputeProgram);
    auto volumeDataSSBO = rlLoadShaderBuffer(volumeBufferSize, cube, RL_STATIC_READ);
    auto volumeDataMaskSSBO = rlLoadShaderBuffer(volumeBufferSize, masks, RL_STATIC_READ);
    rlBindShaderBuffer(volumeDataSSBO, 4);
    rlBindShaderBuffer(volumeDataMaskSSBO, 7);
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
        rlSetUniform(16, &applyMask, RL_SHADER_UNIFORM_INT, 1);
        rlSetUniform(17, maskStrength, RL_SHADER_UNIFORM_FLOAT, 8);
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
    delete[] masks;

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

void loadVolumeData(uint8_t *cube)
{
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
            // -1024 <= pixelVal <= 1023
            int16_t pixelVal = ((int16_t *)imgData)[i];
            if (pixelVal >= -128)
            {
                uint8_t compressed = (uint8_t)((float(pixelVal + 128) / (128 + 1023)) * UINT8_MAX);
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = compressed;
            }
        }
        fileCount++;
    }
    appHelper.Clear();

    // generate filler slices by LERPing actual slices
    const auto layerSize = CUBE_SIZE * CUBE_SIZE;
    for (int i = 0; (i + 4) <= (CUBE_SIZE - 1); i += 4)
    {
        for (int p = 0; p < layerSize; ++p)
        {
            cube[(i + 1) * layerSize + p] =
                (cube[i * layerSize + p] * 3 + cube[(i + 4) * layerSize + p] * 1) / 4;
            cube[(i + 2) * layerSize + p] =
                (cube[i * layerSize + p] * 2 + cube[(i + 4) * layerSize + p] * 2) / 4;
            cube[(i + 3) * layerSize + p] =
                (cube[i * layerSize + p] * 1 + cube[(i + 4) * layerSize + p] * 3) / 4;
        }
    }
}

void loadVolumeMasks(uint8_t *cube)
{
    DICOMAppHelper appHelper;
    DICOMParser parser;

    const std::string BaseFileName = ASSETS_PATH "dicom/LABELLED_DICOM/image_";
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
            // -1024 <= pixelVal <= 1023
            int16_t pixelVal = ((int16_t *)imgData)[i];
            switch (pixelVal)
            {
            case 65: // bone
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 1;
                break;
            case 129: // liver
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 2;
                break;
            case 33: // venous system
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 3;
                break;
            case 17: // portal vein
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 4;
                break;
            case 193: // gallbladder
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 5;
                break;
            case 131: // tumor
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 6;
                break;
            case 133: // liver cyst
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 7;
                break;
            default:
                cube[fileCount * 4 * CUBE_SIZE * CUBE_SIZE + i] = 0;
                break;
            }
        }
        fileCount++;
    }
    appHelper.Clear();
    // generate filler slices by LERPing actual slices
    const auto layerSize = CUBE_SIZE * CUBE_SIZE;
    for (int i = 0; (i + 4) <= (CUBE_SIZE - 1); i += 4)
    {
        for (int p = 0; p < layerSize; ++p)
        {
            cube[(i + 1) * layerSize + p] = cube[i * layerSize + p];
            cube[(i + 2) * layerSize + p] = cube[i * layerSize + p];
            cube[(i + 3) * layerSize + p] = cube[i * layerSize + p];
        }
    }
}

void drawDebugMenu()
{
    rlImGuiBegin();
    ImGui::Begin("Camera Controls");

    // Camera Position Controls
    ImGui::Text("Camera Controls:");

    ImGui::PushID("Controls");
    if (ImGui::DragFloat("RotationX", &camRotateX, 2.0f, -360.0f, 360.0f, "%.2f"))
    {
        camRotateX = std::clamp(camRotateX, -360.0f, 360.0f);
    }
    if (ImGui::DragFloat("RotationY", &camRotateY, 2.0f, -90.0f, 90.0f, "%.2f"))
    {
        camRotateY = std::clamp(camRotateY, -89.9f, 89.9f);
    }
    if (ImGui::DragFloat("FOV", &camera.fovy, 1.0f, 10.0f, 150.0f, "%.1f"))
    {
        camera.fovy = std::clamp(camera.fovy, 10.0f, 150.0f);
    }
    if (ImGui::DragInt("Distance", &zoom, 1, 10, 150))
    {
        zoom = std::clamp(zoom, 10, 150);
    }
    ImGui::PopID();

    // Compute forward direction (camera look-at direction relative to target)
    Vector3 forward = Vector3Normalize(Vector3{
        cosf(camRotateY * DEG2RAD) * sinf(camRotateX * DEG2RAD), // Z-component
        sinf(camRotateY * DEG2RAD),                             // Y-component
        cosf(camRotateY * DEG2RAD) * cosf(camRotateX * DEG2RAD)  // X-component
    });

    // Fixed global up vector (world-space Y-axis)
    Vector3 globalUp = Vector3{0, 1, 0};

    // Compute right vector (perpendicular to forward and global up)
    Vector3 right = Vector3Normalize(Vector3CrossProduct(globalUp, forward));

    // Ensure a stable up vector (recalculate it to be perpendicular to forward and right)
    Vector3 up = Vector3Normalize(Vector3CrossProduct(forward, right));

    // Assign calculated up vector to camera
    camera.up = up;

    // Compute final camera position at a fixed distance (zoom) from target
    camera.position = camera.target - forward * zoom;

    // Image Brightness Control
    ImGui::Text("Brightness:");
    ImGui::PushID("Brightness");
    if (ImGui::DragFloat("Brightness", &brightness, 0.2f, 0.0f, 10.0f, "%.1f"))
    {
        brightness = std::clamp(brightness, 0.0f, 10.0f);
    }
    ImGui::Checkbox("Apply Masks", &applyMask);
    if (applyMask)
    {
        ImGui::SliderFloat("Bones", maskStrength + 1, 0.0f, 1.0f);
        ImGui::SliderFloat("Liver", maskStrength + 2, 0.0f, 1.0f);
        ImGui::SliderFloat("Liver Tumor", maskStrength + 6, 0.0f, 1.0f);
        ImGui::SliderFloat("Liver Cyst", maskStrength + 7, 0.0f, 1.0f);
        ImGui::SliderFloat("Venous System", maskStrength + 3, 0.0f, 1.0f);
        ImGui::SliderFloat("Portal Vein", maskStrength + 4, 0.0f, 1.0f);
        ImGui::SliderFloat("Gallbladder", maskStrength + 5, 0.0f, 1.0f);
    }
    ImGui::PopID();

    ImGui::End();
    rlImGuiEnd();
}