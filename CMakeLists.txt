cmake_minimum_required(VERSION 3.16)
project(DIRECT_VOLUME_RENDERING LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
add_library(raylib_imgui_compiler_flags INTERFACE)
target_compile_features(raylib_imgui_compiler_flags INTERFACE cxx_std_17)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(
        raylib_imgui_compiler_flags
        INTERFACE
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Weffc++;-Wextra;-Wconversion;-Wsign-conversion>>"
        "$<${msvc_cxx}:$<BUILD_INTERFACE:-W4>>")

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}/cmake")
option(RUN_TESTS "Enable tests" ON)
include(coverage)
add_coverage_target("Catch_tests/*")

SET(GRAPHICS "GRAPHICS_API_OPENGL_43")
include(Dependencies.cmake)
raylib_imgui_setup_dependencies()

include(cmake/StaticAnalysers.cmake)

file(GLOB CPU CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/DVR_CPU.cpp")
file(GLOB GPU CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/DVR_GPU.cpp")

add_executable(DVR_CPU ${CPU})
add_executable(DVR_GPU ${GPU})

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(DVR_CPU PUBLIC OpenMP::OpenMP_CXX)
endif()

# TODO: use CPM
add_subdirectory(vendor/DICOMParser)
target_include_directories(DVR_CPU PUBLIC vendor/DICOMParser)
target_include_directories(DVR_GPU PUBLIC vendor/DICOMParser)

target_link_libraries(
        DVR_CPU
        PUBLIC dbg_macro
        fmt
        imgui
        ITKDICOMParser
        raylib
        rlimgui
        spdlog::spdlog_header_only
        raylib_imgui_compiler_flags)

target_link_libraries(
        DVR_GPU
        PUBLIC dbg_macro
        fmt
        imgui
        ITKDICOMParser
        raylib
        rlimgui
        spdlog::spdlog_header_only
        raylib_imgui_compiler_flags)

target_compile_definitions(DVR_CPU PRIVATE SPDLOG_FMT_EXTERNAL)
target_compile_definitions(DVR_GPU PRIVATE SPDLOG_FMT_EXTERNAL)

target_compile_definitions(DVR_CPU PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
target_compile_definitions(DVR_GPU PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/")

target_include_directories(DVR_CPU PUBLIC "${PROJECT_SOURCE_DIR}/src")

if(APPLE)
    target_link_libraries(DVR_CPU PUBLIC "-framework IOKit")
    target_link_libraries(DVR_CPU PUBLIC "-framework Cocoa")
    target_link_libraries(DVR_CPU PUBLIC "-framework OpenGL")

    target_link_libraries(DVR_GPU PUBLIC "-framework IOKit")
    target_link_libraries(DVR_GPU PUBLIC "-framework Cocoa")
    target_link_libraries(DVR_GPU PUBLIC "-framework OpenGL")
endif()

target_include_directories(DVR_CPU PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

option(RUN_UNIT_TESTS "Run Catch2 unit tests" ON)
if(RUN_UNIT_TESTS)
    enable_testing()
    add_subdirectory(Catch_tests)
endif()

# Make this project the startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "DIRECT VOLUME RENDERING")
