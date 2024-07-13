cmake_minimum_required(VERSION 3.20)
project(LearnVulkan VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

add_subdirectory(cmake/Dependencies)

find_package(glfw 3.4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(helloworld src/main.cpp)
target_compile_features(helloworld PRIVATE cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(helloworld
        PRIVATE
            -Wall -Wextra -Wconversion -Wshadow -pedantic
    )
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    target_compile_options(helloworld
        PRIVATE
            -stdlib=libc++
    )
    target_link_options(helloworld
        PRIVATE
            -stdlib=libc++
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
