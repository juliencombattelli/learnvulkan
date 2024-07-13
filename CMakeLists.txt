cmake_minimum_required(VERSION 3.20)
project(VkSponza VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

add_subdirectory(cmake/Dependencies)

find_package(glfw 3.4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(helloworld src/main.cpp)
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
