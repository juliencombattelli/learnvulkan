# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring Stb @ 013ac3be")
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
    GIT_TAG 013ac3be
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(stb)
message(CHECK_PASS "done")

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
