function(include_dependencies)
    set(FILES ${ARGN})
    message(STATUS)
    foreach(FILE IN LISTS FILES)
        include(${FILE})
        message(STATUS)
    endforeach()
endfunction()

include_dependencies(
    FetchGLFW.cmake
    FetchGLM.cmake
    FetchImGui.cmake
    # FetchShaderc.cmake
    FetchSpdlog.cmake
    FetchStb.cmake
    FetchStrongType.cmake
    FetchTinyObjLoader.cmake
)
