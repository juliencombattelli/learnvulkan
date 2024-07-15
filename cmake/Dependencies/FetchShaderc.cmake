# OVERRIDE_FIND_PACKAGE was added in CMake 3.24
cmake_minimum_required(VERSION 3.24)

include(FetchContent)

message(CHECK_START "Fetching and configuring Shaderc v2024.1")
FetchContent_Declare(
    shaderc
    GIT_REPOSITORY https://github.com/google/shaderc
    GIT_TAG v2024.1
    GIT_SHALLOW ON
    OVERRIDE_FIND_PACKAGE
)
set(SHADERC_SKIP_TESTS YES)
set(SHADERC_SKIP_EXAMPLES YES)
set(CMAKE_PROJECT_shaderc_INCLUDE_BEFORE ${CMAKE_CURRENT_LIST_DIR}/Shaderc_IncludeBefore.cmake)
FetchContent_MakeAvailable(shaderc)
message(CHECK_PASS "done")
