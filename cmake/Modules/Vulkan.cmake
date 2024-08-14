# 1.3.268 for full dynamic loading using the DispatchLoaderDynamic
set(MINIMUM_VULKAN_VERSION "1.3.268.0")

set(CMAKE_FIND_DEBUG_MODE NO)

if (CMAKE_CROSSCOMPILING)
    if (CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
        message(FATAL_ERROR "CMAKE_CROSSCOMPILING is set but host and target systems are identical")
    endif()
    message(STATUS "Cross-compile build")
    find_package(Vulkan ${MINIMUM_VULKAN_VERSION})
else()
    message(STATUS "Native build")
    find_package(Vulkan ${MINIMUM_VULKAN_VERSION}
        REQUIRED
        COMPONENTS
            shaderc_combined
    )
    # Shaderc shared library not available as FindVulkan.cmake component
    find_library(shaderc_shared_LIBRARY
        NAMES shaderc_shared
        HINTS
            $ENV{VULKAN_SDK}/lib
            $ENV{VULKAN_SDK}/Lib
        REQUIRED
    )
endif()

set(CMAKE_FIND_DEBUG_MODE NO)
