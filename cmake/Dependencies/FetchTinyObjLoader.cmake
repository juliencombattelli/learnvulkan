# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring TinyObjLoader v1.0.6")
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader
    GIT_TAG v1.0.6
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(tinyobjloader)
message(CHECK_PASS "done")
