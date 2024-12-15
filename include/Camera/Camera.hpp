#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include "Constants.hpp"

#include <raylib.h>
#include <imgui.h>

#include <algorithm>

namespace CameraUtils {
    inline Camera3D camera;

    inline void Initialize()
    {
        camera.position = Vector3{Constants::kCameraPositionX,
                                  Constants::kCameraPositionY,
                                  Constants::kCameraPositionZ};
        camera.target = Vector3{0.F, 0.F, 0.F};
        camera.up = Vector3{0.F, 1.F, 0.F};
        camera.fovy = Constants::kCameraFovY;
        camera.projection = CAMERA_PERSPECTIVE;
    }

    inline void Draw(Camera& camera)
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
        if (ImGui::SliderFloat("FOV", &camera.fovy, 10.0f, 179.9f, "%.1f"))
        {
            // Optional: Logic when FOV changes via slider
        }
        if (ImGui::InputFloat("Manual FOV", &camera.fovy, 0.0f, 0.0f, "%.1f"))
        {
            camera.fovy = std::clamp(camera.fovy, 10.0f, 179.9f);
        }
        ImGui::PopID();
        ImGui::End();
    }
}



#endif //CAMERA_H
