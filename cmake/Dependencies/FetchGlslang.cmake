# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

# TODO detect if VULKAN_SDK is installed and choose the associated glslang tag
message(CHECK_START "Fetching and configuring glslang vulkan-sdk-${VULKAN_MIN_VERSION}")
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG vulkan-sdk-${VULKAN_MIN_VERSION}
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
set(BUILD_SHARED_LIBS NO)
set(BUILD_EXTERNAL NO)
set(SKIP_GLSLANG_INSTALL NO)
set(ENABLE_SPVREMAPPER NO)
set(ENABLE_GLSLANG_BINARIES NO)
set(ENABLE_GLSLANG_JS NO)
set(ENABLE_HLSL NO)
set(ENABLE_OPT NO)
set(ENABLE_CTEST NO)
if(GLSLANG_ENABLE_VALIDATION)
    set(BUILD_EXTERNAL YES)
    set(ENABLE_OPT YES)
    set(CMAKE_PROJECT_glslang_INCLUDE_BEFORE ${CMAKE_CURRENT_LIST_DIR}/Glslang_IncludeBefore.cmake)
endif()
FetchContent_MakeAvailable(glslang)
message(CHECK_PASS "done")

if(NOT GLSLANG_ENABLE_VALIDATION)
    message(WARNING "glslang is built without SPIR-V validation support")
endif()

# Override the targets defined in FindVulkan.cmake to use the fetched glslang instead
if(NOT TARGET Vulkan::glslang-default-resource-limits)
    add_library(Vulkan::glslang-default-resource-limits ALIAS glslang-default-resource-limits)
endif()
if(NOT TARGET Vulkan::glslang-spirv)
    add_library(Vulkan::glslang-spirv ALIAS SPIRV)
endif()
if(NOT TARGET Vulkan::glslang)
    add_library(vulkan_glslang INTERFACE)
    target_link_libraries(vulkan_glslang INTERFACE glslang Vulkan::glslang-spirv)
    add_library(Vulkan::glslang ALIAS vulkan_glslang)
endif()
