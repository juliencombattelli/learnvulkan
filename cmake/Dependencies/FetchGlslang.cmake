# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring glslang vulkan-sdk-1.3.268.0")
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG vulkan-sdk-1.3.268.0
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
set(BUILD_SHARED_LIBS NO)
set(BUILD_EXTERNAL NO)
set(SKIP_GLSLANG_INSTALL YES)
set(ENABLE_SPVREMAPPER NO)
set(ENABLE_GLSLANG_BINARIES NO)
set(ENABLE_GLSLANG_JS NO)
set(ENABLE_HLSL NO)
set(ENABLE_OPT NO)
set(ENABLE_CTEST NO)
FetchContent_MakeAvailable(glslang)
message(CHECK_PASS "done")
