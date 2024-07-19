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

add_library(pch INTERFACE)
target_include_directories(pch INTERFACE src/pch)
target_precompile_headers(pch
    INTERFACE
        src/pch/glfw.hpp
        src/pch/glm.hpp
        src/pch/spdlog.hpp
        src/pch/vulkan.hpp
)
target_link_libraries(pch
    INTERFACE
        glfw
        glm::glm
        spdlog::spdlog
        Vulkan::Vulkan
)

add_executable(helloworld src/main.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(helloworld
        PRIVATE
            -Wall -Wextra -Wconversion -Wshadow -pedantic
    )
endif()
target_link_libraries(helloworld
    PRIVATE
        imgui
        stb
        tinyobjloader
        pch
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
        imgui
        shaderc
        stb
        tinyobjloader
        pch
)
