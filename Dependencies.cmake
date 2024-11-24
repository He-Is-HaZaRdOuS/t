include(cmake/CPM.cmake)

function(raylib_imgui_setup_dependencies)
    # Requires C++ 17
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    message(STATUS "Include Dear ImGui")
    FetchContent_Declare(
        ImGui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG f401021d5a5d56fe2304056c391e78f81c8d4b8f) # v1.91.5
    FetchContent_MakeAvailable(ImGui)
    FetchContent_GetProperties(ImGui SOURCE_DIR IMGUI_DIR)

    add_library(
        imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp)
    target_include_directories(imgui INTERFACE ${imgui_SOURCE_DIR})

    include(cmake/CPM.cmake)

    message(STATUS "Include dbg-macro")
    cpmaddpackage(
        "gh:sharkdp/dbg-macro#fb9976f410f8b29105818b20278cd0be0e853fe8"
    )# v0.5.1

    message(STATUS "Include fmtlib")
    cpmaddpackage("gh:fmtlib/fmt#0c9fce2ffefecfdce794e1859584e25877b7b592"
    )# 11.0.2

    message(STATUS "Include raylib")
    cpmaddpackage("gh:raysan5/raylib#c1ab645ca298a2801097931d1079b10ff7eb9df8"
    )# v5.5

    message(STATUS "Include spdlog")
    #cpmaddpackage("gh:gabime/spdlog#7c02e204c92545f869e2f04edaab1f19fe8b19fd"
    #)# v1.13.0
    cpmaddpackage("gh:gabime/spdlog#27cb4c76708608465c413f6d0e6b8d99a4d84302"
    )# v1.14.1

    message(STATUS "Include rlImGui")
    FetchContent_Declare(
        rlImGui
        GIT_REPOSITORY https://github.com/raylib-extras/rlImGui
        GIT_TAG 583d4fea67e67d431319974f0625f680d3840dfb)
    FetchContent_MakeAvailable(rlImGui)
    FetchContent_GetProperties(rlImGui SOURCE_DIR RLIMGUI_DIR)
    add_library(rlimgui STATIC ${rlimgui_SOURCE_DIR}/rlImGui.cpp)
    target_link_libraries(rlimgui PRIVATE imgui raylib)
    if(APPLE)
        target_link_libraries(rlimgui PUBLIC "-framework IOKit")
        target_link_libraries(rlimgui PUBLIC "-framework Cocoa")
        target_link_libraries(rlimgui PUBLIC "-framework OpenGL")
    endif()
    target_include_directories(rlimgui INTERFACE ${rlimgui_SOURCE_DIR})
endfunction()
