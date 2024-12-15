#pragma once
#ifndef APPLICATION_H
#define APPLICATION_H

#include "Camera/Camera.hpp"
#include "Game/Game.hpp"
#include "Constants.hpp"

#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

namespace Application {
    inline RenderTexture gameTexture;
    inline RenderTexture debugTexture;
    inline Rectangle source_rectangle;
    inline Rectangle destination_rectangle;
    inline Font font;
    inline constexpr Vector2 windowSize{Vector2{Constants::kWindowWidth, Constants::kWindowHeight}};
    inline constexpr float kDebugScaleUp{1.5F};
    inline constexpr double tickTimer{0.0};
    inline constexpr int kMillisecondsPerSecond{1000};

    namespace // Anonymous namespace for private functions
    {
        inline void Update()
        {
            /*
            if (GetTime() - tickTimer >
                static_cast<float>(kMillisecondsPerSecond) /
                    static_cast<float>(Constants::kTickRate) /
                    static_cast<float>(kMillisecondsPerSecond))
            {
                tickTimer = GetTime();
                Game::Update_Debug_Mode();
            }
            */

            Game::Update_Debug_Mode(); // Poll for F9 Key
            Game::Update(CameraUtils::camera, Application::windowSize.x, Application::windowSize.y); // Perform Ray Casting (Build framebuffer)
        }
    }

    inline void Initialize() {
        font = LoadFont(ASSETS_PATH "ibm-plex-mono-v19-latin-500.ttf");

        SetWindowState(FLAG_MSAA_4X_HINT);
        InitWindow(static_cast<int>(windowSize.x),
                   static_cast<int>(windowSize.y),
                   "DVR_CPU");
        rlImGuiSetup(true);

        gameTexture = LoadRenderTexture((int)windowSize.x, (int)windowSize.y);
        debugTexture = LoadRenderTexture(static_cast<int>(windowSize.x / kDebugScaleUp), static_cast<int>(windowSize.y / kDebugScaleUp));

        source_rectangle = Rectangle{0, -windowSize.y, windowSize.x, -windowSize.y};
        destination_rectangle = Rectangle{0, 0, windowSize.x / kDebugScaleUp, windowSize.y / kDebugScaleUp};
    }

    inline int Render()
    {
        // Main Loop
        while (!WindowShouldClose())
        {
            Update();

            BeginDrawing();
            rlImGuiBegin();
            ClearBackground(DARKGRAY);

            if (Game::debugMenu)
            {
                CameraUtils::Draw(CameraUtils::camera); // Draw Camera Controls using ImGui

                BeginTextureMode(Application::gameTexture);
                ClearBackground(RAYWHITE);
                Game::Draw(); // Draw framebuffer
                EndTextureMode();

                BeginTextureMode(Application::debugTexture);
                DrawTexturePro(Application::gameTexture.texture,
                               Application::source_rectangle,
                               Application::destination_rectangle,
                               {0, 0},
                               0.F,
                               RAYWHITE);
                EndTextureMode();

                // Start ImGui
                ImGui::Begin(
                    "Game",
                    &Game::debugMenu,
                    static_cast<uint8_t>( // NOLINT [hicpp-signed-bitwise]
                        ImGuiWindowFlags_AlwaysAutoResize) |
                        static_cast<uint8_t>(ImGuiWindowFlags_NoResize) |
                        static_cast<uint8_t>(ImGuiWindowFlags_NoBackground));
                rlImGuiImageRenderTexture(&Application::debugTexture);
                ImGui::End();
            }
            else
            {
                ClearBackground(RAYWHITE);
                Game::Draw();
            }
            DrawFPS(10, 10);
            rlImGuiEnd();
            EndDrawing();
        }
        return 0;
    }
}

#endif //Application_H
