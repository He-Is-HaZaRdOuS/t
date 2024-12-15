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
#include <filesystem>

#define WIN_WIDTH 1366
#define WIN_HEIGHT 768

Camera3D camera = {.position = {0, 0, -128},
                   .target = {0, 0, 0},
                   .up = {0, 1, 0},
                   .fovy = 45,
                   .projection = CAMERA_PERSPECTIVE};
float camRotateX = 0;
float camRotateY = 0;
float brightness = 1.0f;
bool applyMask = false;
float maskStrength[8] = {0, 0.15f, 0.1f, 0.6f, 1.0f, 0.7f, 0.7f, 0.5f};
int zoom = 128;

std::string BaseFileName;
std::string MaskBaseFileName;
std::string Prefix;
int FileCount;
int SliceThickness;
bool HasMask;
int Width;
int Height;
uint8_t *Volume;
uint8_t *VolumeMask;

void drawDebugMenu();

void loadVolumeData();

void loadVolumeMasks();

void processArgs(int argc, char *argv[]);

std::string extractDirectory(const std::string& imagedir);

std::string inferPrefix(const std::string& directory);

int countFilesWithPrefix(const std::string& directory, const std::string& prefix);

void reorderVolumes();

int main(int argc, char *argv[])
{
    processArgs(argc, argv);
    loadVolumeData();
    if (HasMask)
        loadVolumeMasks();
    reorderVolumes();
    std::cout << "Resolution: " << Width << "x" << Height << "\n";
    std::cout << "Slice Count: " << FileCount << "\n";
    std::cout << "Slice Thickness: " << SliceThickness << "\n";
    std::cout << "Has Mask?: " << (HasMask ? "Yes" : "No") << "\n";

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
    auto volumeBufferSize = Width * Height * FileCount * SliceThickness * sizeof(uint8_t);
    rlEnableShader(dvrComputeProgram);
    auto volumeDataSSBO = rlLoadShaderBuffer(volumeBufferSize, Volume, RL_STATIC_READ);
    auto volumeDataMaskSSBO = rlLoadShaderBuffer(volumeBufferSize, VolumeMask, RL_STATIC_READ);
    rlBindShaderBuffer(volumeDataSSBO, 4);
    rlBindShaderBuffer(volumeDataMaskSSBO, 7);
    int volumeSize[3] = {Width, FileCount * SliceThickness, Height};
    rlSetUniform(5, &volumeSize, RL_SHADER_UNIFORM_IVEC3, 1);

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

    delete[] Volume;
    if (HasMask)
        delete[] VolumeMask;

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

void loadVolumeData()
{
    DICOMAppHelper appHelper;
    DICOMParser parser;

    int currentFile = 0;
    int numPixels;

    while (currentFile < FileCount)
    {
        parser.ClearAllDICOMTagCallbacks();
        parser.OpenFile(BaseFileName + Prefix + std::to_string(currentFile));
        appHelper.Clear();
        appHelper.RegisterCallbacks(&parser);
        appHelper.RegisterPixelDataCallback(&parser);

        parser.ReadHeader();

        void *imgData = nullptr;
        DICOMParser::VRTypes dataType;
        unsigned long imageDataLength = 0;

        appHelper.GetImageData(imgData, dataType, imageDataLength);
        if (currentFile == 0)
        {
            Width = appHelper.GetWidth();
            Height = appHelper.GetHeight();
            numPixels = Width * Height;
            Volume = new uint8_t[numPixels * SliceThickness * FileCount];
        }
        for (size_t i = 0; i < numPixels; ++i)
        {
            // -1024 <= pixelVal <= 1023
            int16_t pixelVal = ((int16_t *)imgData)[i];
            if (pixelVal >= -128)
            {
                uint8_t compressed = (uint8_t)((float(pixelVal + 128) / (128 + 1023)) * UINT8_MAX);
                Volume[currentFile * SliceThickness * numPixels + i] = compressed;
            }
        }
        currentFile++;
    }
    appHelper.Clear();

    // generate filler slices by LERPing actual slices
    for (int i = 0; (i + SliceThickness) < FileCount * SliceThickness; i += SliceThickness)
    {
        for (int l = 1; l < SliceThickness; ++l)
        {
            for (int p = 0; p < numPixels; ++p)
            {
                Volume[(i + l) * numPixels + p] =
                    (Volume[i * numPixels + p] * (SliceThickness - l) +
                     Volume[(i + SliceThickness) * numPixels + p] * l) /
                    SliceThickness;
            }
        }
    }
}

void loadVolumeMasks()
{
    DICOMAppHelper appHelper;
    DICOMParser parser;

    int currentFile = 0;
    int numPixels;
    std::map<uint16_t, Color> masks;

    while (currentFile < FileCount)
    {
        parser.ClearAllDICOMTagCallbacks();
        parser.OpenFile(MaskBaseFileName + Prefix + std::to_string(currentFile));
        appHelper.Clear();
        appHelper.RegisterCallbacks(&parser);
        appHelper.RegisterPixelDataCallback(&parser);

        parser.ReadHeader();

        void *imgData = nullptr;
        DICOMParser::VRTypes dataType;
        unsigned long imageDataLength = 0;

        appHelper.GetImageData(imgData, dataType, imageDataLength);
        if (currentFile == 0)
        {
            numPixels = Width * Height;
            VolumeMask = new uint8_t[numPixels * SliceThickness * FileCount];
        }

        for (size_t i = 0; i < numPixels; ++i)
        {
            // -1024 <= pixelVal <= 1023
            int16_t pixelVal = ((int16_t *)imgData)[i];
            switch (pixelVal)
            {
            case 65: // bone
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 1;
                break;
            case 129: // liver
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 2;
                break;
            case 33: // venous system
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 3;
                break;
            case 17: // portal vein
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 4;
                break;
            case 193: // gallbladder
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 5;
                break;
            case 131: // tumor
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 6;
                break;
            case 133: // liver cyst
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 7;
                break;
            default:
                VolumeMask[currentFile * SliceThickness * numPixels + i] = 0;
                break;
            }
        }
        currentFile++;
    }
    appHelper.Clear();
    // generate filler slices by repeating actual slices
    for (int i = 0; (i + SliceThickness) < FileCount * SliceThickness; i += SliceThickness)
    {
        for (int l = 1; l < SliceThickness; ++l)
        {
            for (int p = 0; p < numPixels; ++p)
            {
                VolumeMask[(i + l) * numPixels + p] = VolumeMask[i * numPixels + p];
            }
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
        sinf(camRotateY * DEG2RAD),                              // Y-component
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
    if (ImGui::Checkbox("Apply Masks", &applyMask))
    {
        if (!HasMask)
            applyMask = false;
    }
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

void processArgs(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::string errMsg = "";
        errMsg += "Expected at least 2 arguments. Usage: ";
        errMsg += "./DVR_GPU slice_thickness base_directory <optional: "
                  "mask_base_directory>\n";
        errMsg += argv[0];
        errMsg += " 4 myDicoms/PATIENT_DICOM/ myDicoms/LABELLED_DICOM/\n";
        throw std::runtime_error(errMsg);
    }
    SliceThickness = (int)ceil(atof(argv[1]));
    BaseFileName = std::string(argv[2]);
    auto dir = extractDirectory(BaseFileName);
    Prefix = inferPrefix(dir);
    FileCount = countFilesWithPrefix(dir, Prefix);
    if (argc == 4)
    {
        MaskBaseFileName = std::string(argv[3]);
        HasMask = true;
    }
}



std::string extractDirectory(const std::string& imagedir) {
    return std::filesystem::path(imagedir).parent_path().string(); // Get the directory part
}

std::string inferPrefix(const std::string& directory) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            const std::string filename = entry.path().filename().string();
            size_t underscorePos = filename.find('_');
            if (underscorePos != std::string::npos) {
                return filename.substr(0, underscorePos + 1); // Include the underscore in the prefix
            }
        }
    }
    throw std::runtime_error("No valid files found to infer the prefix.");
}

int countFilesWithPrefix(const std::string& directory, const std::string& prefix) {
    int count = 0;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            const std::string filename = entry.path().filename().string();
            if (filename.rfind(prefix, 0) == 0) { // Check if filename starts with prefix
                ++count;
            }
        }
    }

    return count;
}

void reorderVolumes()
{
    uint8_t *reorderBuffer = new uint8_t[Width * Height * FileCount * SliceThickness];
    for (int z = 0; z < FileCount * SliceThickness; ++z)
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                reorderBuffer[(y * FileCount * SliceThickness * Width) + (z * Width) + x] =
                    Volume[(z * Height * Width) + (y * Width) + x];
    for (int i = 0; i < Width * Height * FileCount * SliceThickness; ++i)
        Volume[i] = reorderBuffer[i];

    if (!HasMask)
    {
        delete[] reorderBuffer;
        return;
    }
    for (int z = 0; z < FileCount * SliceThickness; ++z)
        for (int y = 0; y < Height; ++y)
            for (int x = 0; x < Width; ++x)
                reorderBuffer[(y * FileCount * SliceThickness * Width) + (z * Width) + x] =
                    VolumeMask[(z * Height * Width) + (y * Width) + x];
    for (int i = 0; i < Width * Height * FileCount * SliceThickness; ++i)
        VolumeMask[i] = reorderBuffer[i];

    delete[] reorderBuffer;
}