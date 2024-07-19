# COMMAND_ERROR_IS_FATAL was added in CMake 3.19
cmake_minimum_required(VERSION 3.19)

message(CHECK_START "Downloading Shaderc dependencies")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND ${Python3_EXECUTABLE} utils/git-sync-deps
    WORKING_DIRECTORY ${shaderc_SOURCE_DIR}
    COMMAND_ECHO STDERR
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
)

# Fail pass is handled with COMMAND_ERROR_IS_FATAL
message(CHECK_PASS "done")
