# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring strong_type v15")
FetchContent_Declare(
    strong_type
    GIT_REPOSITORY https://github.com/rollbear/strong_type
    GIT_TAG v15
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(strong_type)
message(CHECK_PASS "done")
