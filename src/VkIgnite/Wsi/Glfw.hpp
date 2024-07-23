#pragma once

#include "glfw.hpp"

namespace vki::wsi::glfw {

[[nodiscard]] std::vector<const char*> getRequiredExtensions();

[[nodiscard]] vk::UniqueSurfaceKHR createSurfaceKHRUnique(
    const vk::Instance& instance,
    GLFWwindow* window);

} // namespace vki::wsi::glfw
