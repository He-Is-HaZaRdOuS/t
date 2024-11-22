#include "game/game.h"
#include "constants.h"

#include <fmt/format.h>
#include <imgui.h>
#include <raylib.h>

#include <bits/stl_algo.h>
#include <queue>
#include <random>
#include <string>
#include <omp.h>

// Helper function to calculate ray direction from screen coordinates
Vector3 ScreenToRayDirection(int x, int y, const Camera &camera, int screenWidth, int screenHeight)
{
    float aspectRatio = (float)screenWidth / (float)screenHeight;

    float fovAdjustment = tanf(camera.fovy * DEG2RAD / 2.0f);
    float px = ((2.0f * x) / screenWidth - 1.0f) * aspectRatio * fovAdjustment;
    float py = (1.0f - (2.0f * y) / screenHeight) * fovAdjustment;

    Vector3 rayDirection = Vector3Normalize(Vector3Add(camera.target, Vector3Scale(camera.up, py)));
    rayDirection = Vector3Add(rayDirection, Vector3Scale(Vector3CrossProduct(camera.up, camera.target), px));
    return Vector3Normalize(rayDirection);
}

// Helper function to trace a ray through the 3D volume
Color RayCastThroughVolume(const Vector3 &rayOrigin, const Vector3 &rayDir,
                           const std::vector<std::vector<std::vector<int>>> &volume)
{
    // Volume dimensions and step size
    const float cellSize = 1.0f;
    const int volumeSize = constants::kCubeSize;

    // Define cube bounds (assuming cube is centered at origin)
    const float cubeMin = -volumeSize * 0.5f * cellSize;
    const float cubeMax = volumeSize * 0.5f * cellSize;

    // Ray-box intersection
    Vector3 tMin = Vector3Divide(Vector3Subtract(Vector3{cubeMin, cubeMin, cubeMin}, rayOrigin), rayDir);
    Vector3 tMax = Vector3Divide(Vector3Subtract(Vector3{cubeMax, cubeMax, cubeMax}, rayOrigin), rayDir);

    Vector3 tEnter = Vector3Min(tMin, tMax);
    Vector3 tExit = Vector3Max(tMin, tMax);

    float tStart = fmaxf(fmaxf(tEnter.x, tEnter.y), tEnter.z);
    float tEnd = fminf(fminf(tExit.x, tExit.y), tExit.z);

    if (tStart > tEnd || tEnd < 0.0f)
    {
        return BLACK; // No intersection
    }

    // Ray traversal through the volume
    Vector3 currentPosition = Vector3Add(rayOrigin, Vector3Scale(rayDir, tStart));
    float stepSize = 0.1f; // Step size for ray traversal
    Color accumulatedColor = BLACK;

    while (tStart < tEnd)
    {
        // Map world position to volume indices
        int x = (int)((currentPosition.x - cubeMin) / cellSize);
        int y = (int)((currentPosition.y - cubeMin) / cellSize);
        int z = (int)((currentPosition.z - cubeMin) / cellSize);

        if (x >= 0 && x < volumeSize && y >= 0 && y < volumeSize && z >= 0 && z < volumeSize)
        {
            // Get voxel value and map it to a color (grayscale)
            int value = volume[x][y][z];
            Color voxelColor = { (unsigned char)value, (unsigned char)value, (unsigned char)value, 255 };

            // Accumulate color (simple maximum intensity projection)
            accumulatedColor = ColorAdd(accumulatedColor, voxelColor);
        }

        // Advance ray position
        currentPosition = Vector3Add(currentPosition, Vector3Scale(rayDir, stepSize));
        tStart += stepSize;
    }

    return accumulatedColor;
}

void DrawRaycastTexture(Texture2D &raycastTexture)
{
    // Draw the updated texture to the screen (scaled to the full screen size)
    DrawTexture(raycastTexture, 0, 0, WHITE);
}


void RenderVolumeRaycastingToImage(const std::vector<std::vector<std::vector<int>>> &volume,
                                    const Camera &camera, int screenWidth, int screenHeight,
                                    Image &raycastImage, Texture2D &raycastTexture)
{
    // Access the pixel data of the image
    Color* pixels = (Color*)raycastImage.data;

    // Parallelize raycasting for the whole grid of pixels
#pragma omp parallel for
    for (int y = 0; y < screenHeight; ++y)
    {
        for (int x = 0; x < screenWidth; ++x)
        {
            // Calculate the ray direction based on the camera and pixel coordinates
            Vector3 rayDir = ScreenToRayDirection(x, y, camera, screenWidth, screenHeight);

            // Perform raycasting through the 3D volume to get the color for this pixel
            Color pixelColor = RayCastThroughVolume(camera.position, rayDir, volume);

            // Store the color directly in the image's pixel data
            pixels[y * screenWidth + x] = pixelColor;
        }
    }

    // Once the image is populated, we need to update the texture
    UpdateTexture(raycastTexture, pixels); // Upload the pixel data to the texture
}

// Function to handle camera controls using ImGui
void Camera_Controls(Camera &camera)
{
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
    if (ImGui::SliderFloat("FOV", &camera.fovy, 10.0f, 90.0f, "%.1f"))
    {
        // Optional: Logic when FOV changes via slider
    }
    if (ImGui::InputFloat("Manual FOV", &camera.fovy, 0.0f, 0.0f, "%.1f"))
    {
        camera.fovy = std::clamp(camera.fovy, 10.0f, 90.0f);
    }
    ImGui::PopID();

    ImGui::End();
}


// Function to generate random 3D cube data
void GenerateRandomCubeData(std::vector<std::vector<std::vector<int>>> &cube)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, constants::kMaxRandomValue);

    for (int x = 0; x < constants::kCubeSize; ++x)
    {
        for (int y = 0; y < constants::kCubeSize; ++y)
        {
            for (int z = 0; z < constants::kCubeSize; ++z)
            {
                cube[x][y][z] = dist(rng);
            }
        }
    }
}

// Function to render the cube data using raycasting
void RenderCubeData(const std::vector<std::vector<std::vector<int>>> &cube, const Camera &camera)
{
    BeginMode3D(camera);

    const float cellSize = 1.0f; // Size of each cube cell

    for (int x = 0; x < constants::kCubeSize; ++x)
    {
        for (int y = 0; y < constants::kCubeSize; ++y)
        {
            for (int z = 0; z < constants::kCubeSize; ++z)
            {
                int value = cube[x][y][z];

                // Map the value to a color (grayscale)
                Color cellColor = { static_cast<unsigned char>(value),
                                    static_cast<unsigned char>(value),
                                    static_cast<unsigned char>(value),
                                    255 };

                // Render the cube cell
                Vector3 position = { x * cellSize, y * cellSize, z * cellSize };
                DrawCube(position, cellSize, cellSize, cellSize, cellColor);
            }
        }
    }

    EndMode3D();
}

void draw_scene(const Camera &camera,
                const Vector3 &cube_position,
                const Color &cube_colour,
                const Font &font)
{
    BeginMode3D(camera);
    DrawCube(cube_position,
             constants::kCubeSizeLength,
             constants::kCubeSizeLength,
             constants::kCubeSizeLength,
             cube_colour);
    DrawGrid(constants::kGridSlices, constants::kGridSpacing);
    DrawCubeWires(cube_position,
                  constants::kCubeSizeLength,
                  constants::kCubeSizeLength,
                  constants::kCubeSizeLength,
                  DARKGRAY);
    EndMode3D();
    const float kDefaultFontSize{10.F};
    DrawTextEx(font,
               "Press F9 for ImGui debug mode",
               Vector2{constants::kTextPositionX, constants::kTextPositionY},
               static_cast<float>(constants::kTextFontSize),
               static_cast<float>(constants::kTextFontSize) / kDefaultFontSize,
               DARKGRAY);
    DrawFPS(constants::kFPSPositionX, constants::kFPSPositionY);
}

int Game_GetTickrate()
{
    constexpr int kTickrate{128};
    return kTickrate;
}

void Game_Update(std::queue<int> *key_queue, bool *debug_menu)
{
    for (; !key_queue->empty(); key_queue->pop())
    {
        const int key{key_queue->front()};
        if (key == 0)
        {
            continue;
        }
        if (key == KEY_F9)
        {
            *(debug_menu) = !(*debug_menu);
        }
    }
}

void Game_DrawDebug(int &selected_cube_colour)
{
    ImGui::Begin("Game Tests");

    ImGui::Text("%s", // NOLINT [cppcoreguidelines-pro-type-vararg]
                fmt::format("FPS: {}", GetFPS()).c_str());

    if (ImGui::TreeNode("Cube colour"))
    {
        int index{0};
        for (const std::string &colour : constants::kCubeColourLabels)
        {
            ImGui::RadioButton(colour.c_str(), &selected_cube_colour, index);
            ++index;
        }
    }
    ImGui::End();
}
