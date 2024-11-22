#include "constants.h"
#include "game/game.h"

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <string>

void setup_camera(Camera3D &camera)
{
    camera.position = Vector3{constants::kCameraPositionX,
                              constants::kCameraPositionY,
                              constants::kCameraPositionZ};
    constexpr float kCameraTargetX{0.F};
    camera.target = Vector3{kCameraTargetX, 0.F, 0.F};
    camera.up = Vector3{0.F, 1.F, 0.F};
    camera.fovy = constants::kCameraFovY;
    camera.projection = CAMERA_PERSPECTIVE;
}

int main()
{
    double tickTimer{0.0};
    std::queue<int> keyQueue = std::queue<int>();
    bool debugMenu = false;
    const Vector2 windowSize{
        Vector2{constants::kWindowWidth, constants::kWindowHeight}};
    RenderTexture gameTexture;
    RenderTexture debugTexture;

    SetWindowState(FLAG_MSAA_4X_HINT);
    InitWindow(static_cast<int>(windowSize.x),
               static_cast<int>(windowSize.y),
               constants::kTitle.c_str());
    rlImGuiSetup(true);

    gameTexture = LoadRenderTexture((int)windowSize.x, (int)windowSize.y);
    constexpr float kDebugScaleUp{1.5F};
    debugTexture =
        LoadRenderTexture(static_cast<int>(windowSize.x / kDebugScaleUp),
                          static_cast<int>(windowSize.y / kDebugScaleUp));

    const Rectangle source_rectangle{0,
                                     -windowSize.y,
                                     windowSize.x,
                                     -windowSize.y};
    const Rectangle destination_rectangle{0,
                                          0,
                                          windowSize.x / kDebugScaleUp,
                                          windowSize.y / kDebugScaleUp};
    Camera3D camera{};
    setup_camera(camera);

    constexpr int kMillisecondsPerSecond{1000};
    Vector3 cube_position{0.F, 0.F, 0.F};
    const Vector3 cube_velocity{Vector3{0.F, 0.F, constants::kCubeSpeed}};
    const Font font{LoadFont(ASSETS_PATH "ibm-plex-mono-v19-latin-500.ttf")};
    int selected_cube_colour{0};

    std::vector<std::vector<std::vector<int>>> cube(constants::kCubeSize,
                                                     std::vector<std::vector<int>>(constants::kCubeSize,
                                                                                   std::vector<int>(constants::kCubeSize)));
    GenerateRandomCubeData(cube);
    Image raycastImage;       // Holds pixel data
    Texture2D raycastTexture; // The texture that will be rendered

    // Allocate memory for the image
    raycastImage = GenImageColor(windowSize.x, windowSize.y, RAYWHITE); // Start with a blank white image

    // Load this image into a texture
    raycastTexture = LoadTextureFromImage(raycastImage);  // Convert image to texture


    while (!WindowShouldClose())
    {
        const float frame_time{GetFrameTime()};

        if (GetTime() - tickTimer >
            static_cast<float>(kMillisecondsPerSecond) /
                static_cast<float>(constants::kTickrate) /
                static_cast<float>(kMillisecondsPerSecond))
        {
            tickTimer = GetTime();
            Game_Update(&keyQueue, &debugMenu);
        }

        keyQueue.push(GetKeyPressed());
        RenderVolumeRaycastingToImage(cube, camera, windowSize.x, windowSize.y, raycastImage, raycastTexture);

        BeginDrawing();
        rlImGuiBegin();
        ClearBackground(DARKGRAY);

        if (debugMenu)
        {
            Camera_Controls(camera);
            BeginTextureMode(gameTexture);
            ClearBackground(RAYWHITE);
            DrawRaycastTexture(raycastTexture); // Draw the texture with the raycast result
            EndTextureMode();
            BeginTextureMode(debugTexture);
            DrawTexturePro(gameTexture.texture,
                           source_rectangle,
                           destination_rectangle,
                           {0, 0},
                           0.F,
                           RAYWHITE);
            EndTextureMode();

            Game_DrawDebug(selected_cube_colour);

            ImGui::Begin(
                "Game",
                &debugMenu,
                static_cast<uint8_t>( // NOLINT [hicpp-signed-bitwise]
                    ImGuiWindowFlags_AlwaysAutoResize) |
                    static_cast<uint8_t>(ImGuiWindowFlags_NoResize) |
                    static_cast<uint8_t>(ImGuiWindowFlags_NoBackground));
            rlImGuiImageRenderTexture(&debugTexture);
            ImGui::End();
        }
        else
        {
            ClearBackground(RAYWHITE);
            DrawRaycastTexture(raycastTexture); // Draw the texture with the raycast result
        }
        DrawFPS(10, 10);
        rlImGuiEnd();
        EndDrawing();

    }
    return 0;
}
