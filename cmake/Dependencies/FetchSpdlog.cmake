# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring Spdlog v1.14.1")
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.14.1
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(spdlog)
message(CHECK_PASS "done")
