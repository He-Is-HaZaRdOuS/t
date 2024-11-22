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
void Camera_Controls(Camera &camera);
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

// Normalize a vector to have unit length
inline Vector3 Vector3Normalize(const Vector3 &v)
{
  float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
  if (length == 0.0f)
    return Vector3{0.0f, 0.0f, 0.0f}; // Return zero vector if the input vector is zero
  return Vector3{v.x / length, v.y / length, v.z / length};
}

// Compute the cross product of two vectors
inline Vector3 Vector3CrossProduct(const Vector3 &v1, const Vector3 &v2)
{
  return Vector3{
    v1.y * v2.z - v1.z * v2.y,
    v1.z * v2.x - v1.x * v2.z,
    v1.x * v2.y - v1.y * v2.x};
}

// Vector addition
inline Vector3 Vector3Add(const Vector3 &v1, const Vector3 &v2)
{
  return Vector3{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

// Vector subtraction
inline Vector3 Vector3Subtract(const Vector3 &v1, const Vector3 &v2)
{
  return Vector3{v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

// Vector multiplication (component-wise)
inline Vector3 Vector3Multiply(const Vector3 &v1, const Vector3 &v2)
{
  return Vector3{v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

// Vector division (component-wise)
inline Vector3 Vector3Divide(const Vector3 &v1, const Vector3 &v2)
{
  return Vector3{v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
}

// Scalar multiplication
inline Vector3 Vector3Scale(const Vector3 &v, float scalar)
{
  return Vector3{v.x * scalar, v.y * scalar, v.z * scalar};
}

// Component-wise min
inline Vector3 Vector3Min(const Vector3 &v1, const Vector3 &v2)
{
  return Vector3{
    fminf(v1.x, v2.x),
    fminf(v1.y, v2.y),
    fminf(v1.z, v2.z)};
}

// Component-wise max
inline Vector3 Vector3Max(const Vector3 &v1, const Vector3 &v2)
{
  return Vector3{
    fmaxf(v1.x, v2.x),
    fmaxf(v1.y, v2.y),
    fmaxf(v1.z, v2.z)};
}

// Implementation of Vector3Distance
inline float Vector3Distance(const Vector3& v1, const Vector3& v2)
{
  return sqrtf((v2.x - v1.x) * (v2.x - v1.x) +
               (v2.y - v1.y) * (v2.y - v1.y) +
               (v2.z - v1.z) * (v2.z - v1.z));
}

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
