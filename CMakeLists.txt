cmake_minimum_required(VERSION 3.20)
project(LearnVulkan VERSION 1.0.0)

# Use RelWithDebInfo as default build configuration
if(CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
    # For Ninja Multi-Config generator
    set(CMAKE_DEFAULT_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()
if(NOT CMAKE_BUILD_TYPE)
    # For single-config generators
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(VULKAN_MIN_VERSION_MAJOR 1)
set(VULKAN_MIN_VERSION_MINOR 3)
set(VULKAN_MIN_VERSION_PATCH 268)
set(VULKAN_MIN_VERSION "${VULKAN_MIN_VERSION_MAJOR}.${VULKAN_MIN_VERSION_MINOR}.${VULKAN_MIN_VERSION_PATCH}.0")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

include(ForceColoredOutput)

add_subdirectory(cmake/Dependencies)

find_package(glfw 3.4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(strong_type CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Vulkan ${VULKAN_MIN_VERSION} MODULE
    REQUIRED
    COMPONENTS
        glslang glslang-spirv SPIRV-Tools SPIRV-Tools-opt
)

# Build ImGui
add_library(imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui
    PUBLIC
        Vulkan::Headers
)

# Build precompiled headers
add_library(pch INTERFACE)
target_include_directories(pch INTERFACE src)
target_precompile_headers(pch
    INTERFACE
        src/Pch/Glfw.hpp
        src/Pch/Glm.hpp
        src/Pch/Spdlog.hpp
        src/Pch/Vulkan.hpp
)
target_link_libraries(pch
    INTERFACE
        glfw
        glm::glm
        spdlog::spdlog
        # Currently vulkan is loaded dynamically by the application, so only headers are needed
        Vulkan::Headers
)

# Build main application
configure_file(src/VkIgnite/MinVkVersion.hpp.in MinVkVersion.hpp @ONLY)
add_executable(helloworld
    src/main.cpp
    src/VkIgnite/Wsi/Glfw.cpp
    src/VkIgnite/VkIgnite.cpp
    src/VkIgnite/Shader.cpp
    src/VkIgnite/Instance.cpp
)
target_include_directories(helloworld PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID MATCHES Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(helloworld
        PRIVATE
            -Wall -Wextra -Wconversion -Wshadow -pedantic
    )
endif()
target_link_libraries(helloworld
    PRIVATE
        pch
        imgui
        stb
        strong_type
        tinyobjloader
        Vulkan::glslang
        Vulkan::glslang-default-resource-limits
)

# Not buildable due to Shaderc dependency
# Build sample application
# add_executable(vulkan-hpp-test src/vulkan-hpp-test.cpp)
# target_link_libraries(vulkan-hpp-test
#     PRIVATE
#         imgui
#         ${shaderc_shared_LIBRARY}
#         stb
#         tinyobjloader
#         pch
# )
