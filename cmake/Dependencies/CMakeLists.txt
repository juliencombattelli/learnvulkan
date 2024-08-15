message(STATUS)
function(include_dependencies)
    set(FILES ${ARGN})
    foreach(FILE IN LISTS FILES)
        include(${FILE})
        message(STATUS)
    endforeach()
endfunction()

# Dependencies not provided by Vulkan SDK
include_dependencies(
    FetchGLFW.cmake
    FetchGLM.cmake
    FetchImGui.cmake
    FetchSpdlog.cmake
    FetchStb.cmake
    FetchStrongType.cmake
    FetchTinyObjLoader.cmake
)

if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    message(STATUS "Building with Clang's libc++ support")
    # The Vulkan SDK prebuilts for Linux are linked with the GNU libstdc++
    # So when using Clang's libc++ we build those packages from source
    set(FETCH_GLSLANG YES)
endif()

if(MINGW)
    # Due to MSVC ABI shenanigans, glslang from Windows Vulkan SDK cannot be linked with MinGW
    # So we build it from source
    set(FETCH_GLSLANG YES)
endif()

if(FETCH_GLSLANG)
    include_dependencies(
        FetchGlslang.cmake
    )
endif()
