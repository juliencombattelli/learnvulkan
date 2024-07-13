# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring GLM 1.0.1")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 1.0.1
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glm)
message(CHECK_PASS "done")
