#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <raylib.h>

#include <array>
#include <string>

namespace Constants
{
inline const std::string kTitle{"Direct Volume Rendering : Raylib + ImGui"};
inline constexpr int kWindowWidth{1366};
inline constexpr int kWindowHeight{768};
inline constexpr int kCubeSize{32}; // Cube dimensions
inline constexpr int kMaxRandomValue{255}; // Maximum random value in the cube
inline constexpr int kTickRate{128};
inline constexpr float kCameraPositionX{0.F};
inline constexpr float kCameraPositionY{0.F};
inline constexpr float kCameraPositionZ{-30.F};
inline constexpr float kCameraFovY{120.F};
inline constexpr float kCubeSizeLength{2.F};
inline constexpr int kGridSlices{10};
inline constexpr float kCubeSpeed{1.2F};
inline constexpr float kCubePositionMinZ{-5.F};
inline constexpr float kCubePositionMaxZ{5.F};
inline constexpr int kIndigoR{75};
inline constexpr int kIndigoG{0};
inline constexpr int kIndigoB{130};
inline constexpr int kIndigoA{255};
inline constexpr std::array<Color, 7> kCubeColours{
    RED,
    ORANGE,
    YELLOW,
    GREEN,
    BLUE,
    Color{kIndigoR, kIndigoG, kIndigoB, kIndigoA},
    VIOLET};
inline const std::array<std::string, 7> kCubeColourLabels{"Red",
                                                          "Orange",
                                                          "Yellow",
                                                          "Green",
                                                          "Blue",
                                                          "Indigo",
                                                          "Violet"};
inline constexpr float kGridSpacing{1.F};
inline constexpr int kTextPositionX{10};
inline constexpr int kTextPositionY{40};
inline constexpr int kTextFontSize{24};
inline constexpr int kFPSPositionX{10};
inline constexpr int kFPSPositionY{10};
inline constexpr int kOMPThreads{8};
} // namespace constants

#endif
