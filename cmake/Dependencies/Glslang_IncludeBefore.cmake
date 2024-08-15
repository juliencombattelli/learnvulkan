# COMMAND_ERROR_IS_FATAL was added in CMake 3.19
cmake_minimum_required(VERSION 3.19)

message(CHECK_START "Downloading Glslang dependencies")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND ${Python3_EXECUTABLE} update_glslang_sources.py
    WORKING_DIRECTORY ${glslang_SOURCE_DIR}
    COMMAND_ECHO STDERR
    COMMAND_ERROR_IS_FATAL ANY
    OUTPUT_QUIET
    ERROR_QUIET
)

# Fail pass is handled with COMMAND_ERROR_IS_FATAL
message(CHECK_PASS "done")
