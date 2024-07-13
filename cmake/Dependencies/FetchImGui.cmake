# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring ImGui v1.90.9")
FetchContent_Declare(imgui
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.90.9
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(imgui)
message(CHECK_PASS "done")

add_library(imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
