//
// Created by nosquad on 12/29/2023.
//

#ifndef BASE_RAYLIB_GAME_PROJECT_GAME_H
#define BASE_RAYLIB_GAME_PROJECT_GAME_H
#include <cmath>
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

#include <cstdio>
#include <queue>

Vector3 ScreenToRayDirection(int x, int y, const Camera &camera, int screenWidth, int screenHeight);
Color RayCastThroughVolume(const Vector3 &rayOrigin, const Vector3 &rayDir, const std::vector<std::vector<std::vector<int>>> &volume);
void DrawRaycastTexture(Texture2D &raycastTexture);
void RenderVolumeRaycastingToImage(const std::vector<std::vector<std::vector<int>>> &volume,
                                    const Camera &camera, int screenWidth, int screenHeight,
                                    Image &raycastImage, Texture2D &raycastTexture);
void UpdateCamera(Camera &camera);
void GenerateRandomCubeData(std::vector<std::vector<std::vector<int>>> &cube);
void RenderCubeData(const std::vector<std::vector<std::vector<int>>> &cube, const Camera &camera);
void draw_scene(const Camera &camera,
                const Vector3 &cube_position,
                const Color &cube_colour,
                const Font &font);
//int Game_GetTickrate();
void Game_Update(std::queue<int> *key_queue, bool *debug_menu);
void Game_Draw();
void Game_DrawDebug(int &selected_cube_colour);

// Clamp value between 0 and 255
static unsigned char ClampColorValue(int value)
{
  if (value < 0) return 0;
  if (value > 255) return 255;
  return (unsigned char)value;
}

// Add two colors (component-wise) with clamping
inline Color ColorAdd(const Color &c1, const Color &c2)
{
  return Color{
    ClampColorValue(c1.r + c2.r),
    ClampColorValue(c1.g + c2.g),
    ClampColorValue(c1.b + c2.b),
    ClampColorValue(c1.a + c2.a)};
}

#endif //BASE_RAYLIB_GAME_PROJECT_GAME_H
