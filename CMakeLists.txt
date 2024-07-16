cmake_minimum_required(VERSION 3.20)
project(LearnVulkan VERSION 1.0.0)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi -fuse-ld=lld")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi -fuse-ld=lld")
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

add_subdirectory(cmake/Dependencies)

find_package(glfw 3.4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(helloworld src/main.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(helloworld
        PRIVATE
            -Wall -Wextra -Wconversion -Wshadow -pedantic
    )
endif()
target_link_libraries(helloworld
    PRIVATE
        glfw
        glm::glm
        imgui
        spdlog::spdlog
        stb
        tinyobjloader
        Vulkan::Vulkan
)
target_compile_definitions(helloworld
    PRIVATE
        VULKAN_HPP_NO_CONSTRUCTORS
        VULKAN_HPP_NO_TO_STRING
        VULKAN_HPP_HAS_SPACESHIP_OPERATOR
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        GLFW_INCLUDE_NONE
)
target_precompile_headers(helloworld
    PUBLIC
        <vulkan/vulkan.hpp>
        <GLFW/glfw3.h>
        <glm/glm.hpp>
        <spdlog/spdlog.h>
        <algorithm>
        <cstdlib>
        <iostream>
        <ranges>
        <stdexcept>
        <string_view>
        <vector>
)

add_executable(vulkan-hpp-test src/vulkan-hpp-test.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(vulkan-hpp-test
        PRIVATE
            -Wall -Wextra -Wconversion -Wshadow -pedantic
    )
endif()
target_link_libraries(vulkan-hpp-test
    PRIVATE
        glfw
        glm::glm
        imgui
        shaderc
        spdlog::spdlog
        stb
        tinyobjloader
        Vulkan::Vulkan
)
target_compile_definitions(vulkan-hpp-test
    PRIVATE
        VULKAN_HPP_NO_CONSTRUCTORS
        GLFW_INCLUDE_NONE
)
target_precompile_headers(vulkan-hpp-test
    PUBLIC
        <vulkan/vulkan.hpp>
        <GLFW/glfw3.h>
        <glm/glm.hpp>
        <spdlog/spdlog.h>
        <cstdlib>
        <stdexcept>
        <string_view>
        <vector>
        <iostream>
)
