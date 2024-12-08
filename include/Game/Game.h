#pragma once
#ifndef GAME_H
#define GAME_H

#include "Constants.h"
#include "DICOMAppHelper.h"
#include "DICOMParser.h"

#include <fmt/format.h>
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <omp.h>

#include <algorithm>
#include <queue>
#include <random>
#include <string>

namespace Game
{
    inline Image raycastImage;       // Holds pixel data
    inline Texture2D raycastTexture; // The texture that will be rendered
    inline std::queue<int> keyQueue = std::queue<int>();
    inline bool debugMenu = false;

    inline std::vector<std::vector<std::vector<int>>> cube(Constants::kCubeSize, std::vector<std::vector<int>>(Constants::kCubeSize, std::vector<int>(Constants::kCubeSize)));

    // Anonymous namespace for private functions
    namespace
    {
        // Clamp value between 0 and 255
        inline unsigned char ClampColorValue(int value)
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

        // Helper function to calculate ray direction from screen coordinates
        Vector3 ScreenToRayDirection(int x, int y, const Camera &camera, int screenWidth, int screenHeight)
        {
            // "normalize" x and y: [0, screenWidth or screenHeight] -> [-1,1]
            float normX = (((float)x / (float)screenWidth) - 0.5f) * 2.0f;
            float normY = (((float)y / (float)screenHeight) - 0.5f) * 2.0f;
            float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);

            Vector3 cameraDirection = Vector3Normalize(camera.target - camera.position);
            Vector3 horizontal = Vector3CrossProduct(camera.up, cameraDirection);

            // distance between camera & perspective plane = cot(fov / 2)
            float d = 1.0f / tanf((camera.fovy * DEG2RAD) * 0.5f);
            Vector3 rayDirection = cameraDirection * d + camera.up * (-normY) + horizontal * (normX * aspectRatio);

            return Vector3Normalize(rayDirection);
        }

        // Helper function to trace a ray through the 3D volume
        Color RayCastThroughVolume(const Vector3 &rayOrigin, const Vector3 &rayDir,
                                   const std::vector<std::vector<std::vector<int>>> &volume)
        {
            const float cellSize = 1.0f;
            const int volumeSize = Constants::kCubeSize;

            // Define cube bounds (assuming cube is centered at origin)
            const float cubeMin = -volumeSize * 0.5f * cellSize;
            const float cubeMax = volumeSize * 0.5f * cellSize;

            // Ray-box intersection
            Vector3 invRayDir = {
                (rayDir.x != 0.0f) ? 1.0f / rayDir.x : INFINITY,
                (rayDir.y != 0.0f) ? 1.0f / rayDir.y : INFINITY,
                (rayDir.z != 0.0f) ? 1.0f / rayDir.z : INFINITY
            };

            Vector3 tMin = (Vector3{cubeMin, cubeMin, cubeMin} - rayOrigin) * invRayDir;
            Vector3 tMax = (Vector3{cubeMax, cubeMax, cubeMax} - rayOrigin) * invRayDir;

            Vector3 tEnter = Vector3Min(tMin, tMax);
            Vector3 tExit = Vector3Max(tMin, tMax);

            float tStart = std::max({ tEnter.x, tEnter.y, tEnter.z });
            float tEnd = std::min({ tExit.x, tExit.y, tExit.z });

            if (tStart > tEnd || tEnd < 0.0f)
            {
                return BLACK; // No intersection
            }

            // Ray traversal through the volume
            Vector3 currentPosition = rayOrigin + rayDir * tStart;
            float stepSize = 0.1f; // Step size for ray traversal
            float maxAlpha = 0.0f;
            Color accumulatedColor = BLACK;

            while (tStart < tEnd)
            {
                // Map world position to volume indices
                int x = static_cast<int>((currentPosition.x - cubeMin) / cellSize);
                int y = static_cast<int>((currentPosition.y - cubeMin) / cellSize);
                int z = static_cast<int>((currentPosition.z - cubeMin) / cellSize);

                if (x >= 0 && x < volumeSize && y >= 0 && y < volumeSize && z >= 0 && z < volumeSize)
                {
                    // Get voxel value and map it to a color (grayscale)
                    int value = volume[x][y][z];
                    float voxelAlpha = float(value) / 255.0f;
                    Color voxelColor = { static_cast<unsigned char>(value),
                                         static_cast<unsigned char>(value),
                                         static_cast<unsigned char>(value),
                                         static_cast<unsigned char>(voxelAlpha) };

                    // Accumulate color (simple maximum intensity projection)
                    accumulatedColor = ColorAdd(accumulatedColor, voxelColor);

                    // maximum intensity projection
                    if (voxelAlpha > maxAlpha) {
                        maxAlpha = voxelAlpha;
                    }
                }

                // Advance ray position
                currentPosition += rayDir * stepSize;
                tStart += stepSize;
            }

            return accumulatedColor;
            return Color{1, 1, 1, (unsigned char)maxAlpha};
        }

        // Function to generate random 3D cube data
        void GenerateRandomCubeData()
        {
            std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(0, Constants::kMaxRandomValue);

            for (int x = 0; x < Constants::kCubeSize; ++x)
            {
                for (int y = 0; y < Constants::kCubeSize; ++y)
                {
                    for (int z = 0; z < Constants::kCubeSize; ++z)
                    {
                        cube[x][y][z] = dist(rng);
                    }
                }
            }
        }

        // Function to generate 3D cube data with checker pattern
        void GenerateCheckerCubeData()
        {
            for (int x = 0; x < Constants::kCubeSize; ++x)
            {
                for (int y = 0; y < Constants::kCubeSize; ++y)
                {
                    for (int z = 0; z < Constants::kCubeSize; ++z)
                    {
                        cube[x][y][z] = 2 * ((x+y+z) % 2 == 0);
                    }
                }
            }
        }

        void loadVolumeData(std::vector<std::vector<std::vector<int>>>& cube)
        {
            DICOMAppHelper appHelper;
            DICOMParser parser;

            const std::string BaseFileName = ASSETS_PATH "dicom/PATIENT_DICOM/image_";
            int fileCount = 0;
            int maxFileCount = 124;

            int kCubeSize = Constants::kCubeSize;

            while (fileCount < maxFileCount)
            {
                parser.ClearAllDICOMTagCallbacks();
                parser.OpenFile(BaseFileName + std::to_string(fileCount));
                appHelper.Clear();
                appHelper.RegisterCallbacks(&parser);
                appHelper.RegisterPixelDataCallback(&parser);

                parser.ReadHeader();

                void* imgData = nullptr;
                DICOMParser::VRTypes dataType;
                unsigned long imageDataLength = 0;

                appHelper.GetImageData(imgData, dataType, imageDataLength);
                auto numPixels = imageDataLength / 2;

                for (size_t i = 0; i < kCubeSize; ++i)
                {
                    for (size_t j = 0; j < kCubeSize; ++j)
                    {
                        int x = (i / kCubeSize) * 512;
                        int y = (j / kCubeSize) * 512;
                        int16_t pixelVal = ((int16_t*)imgData)[x + y * 512];
                        if (pixelVal > 0)
                        {
                            uint8_t compressed = static_cast<uint8_t>((float(pixelVal) / float(INT16_MAX)) * UINT8_MAX);

                            // Assign compressed values to the corresponding slices
                            for (int slice = 0; slice < 4; ++slice)
                            {
                                if (fileCount + slice < maxFileCount) {
                                    cube[fileCount + slice][i][j] = compressed;
                                }
                            }
                        }
                    }
                }
                fileCount += 4;
            }
        }

    } // Anonymous namespace

    inline void Initialize(Vector2 windowSize)
    {
        //GenerateRandomCubeData();
        GenerateCheckerCubeData();

        // Initialize the cube with some values
            // for (int x = 0; x < Constants::kCubeSize; ++x) {
            //     for (int y = 0; y < Constants::kCubeSize; ++y) {
            //         for (int z = 0; z < Constants::kCubeSize; ++z) {
            //             cube[x][y][z] = x * Constants::kCubeSize * Constants::kCubeSize + y * Constants::kCubeSize + z;
            //         }
            //     }
            // }
            // loadVolumeData(cube);
        raycastImage = GenImageColor(windowSize.x, windowSize.y, RAYWHITE); // Start with a blank white image
        raycastTexture = LoadTextureFromImage(raycastImage);  // Convert image to texture
    }

    inline void Draw()
    {
        // Draw the updated texture to the screen (scaled to the full screen size)
        DrawTexture(raycastTexture, 0, 0, WHITE);
    }

    // Function to render raycasting to a texture
    inline void Update(const Camera &camera, int screenWidth, int screenHeight)
    {
        // Access the pixel data of the image
        Color* pixels = reinterpret_cast<Color*>(raycastImage.data);

        // Parallelize raycasting for the whole grid of pixels
    #pragma omp parallel for num_threads(Constants::kOMPThreads) schedule(guided)
        for (int y = 0; y < screenHeight; ++y)
        {
            for (int x = 0; x < screenWidth; ++x)
            {
                // Calculate the ray direction based on the camera and pixel coordinates
                Vector3 rayDir = ScreenToRayDirection(x, y, camera, screenWidth, screenHeight);

                // Perform raycasting through the 3D volume (cube) to get the color for this pixel
                Color pixelColor = RayCastThroughVolume(camera.position, rayDir, cube);

                // Store the color directly in the image's pixel data
                pixels[y * screenWidth + x] = pixelColor;
            }
        }

        // Once the image is populated, we need to update the texture
        UpdateTexture(raycastTexture, pixels); // Upload the pixel data to the texture
    }

    inline void Update_Debug_Mode()
    {
        // Poll keyboard input
        keyQueue.push(GetKeyPressed());

        for (; !keyQueue.empty(); keyQueue.pop())
        {
            const int key{keyQueue.front()};
            if (key == 0)
            {
                continue;
            }
            if (key == KEY_F9)
            {
                debugMenu = !debugMenu;
            }
        }
    }
}
#endif
