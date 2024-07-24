#pragma once

#include "VkIgnite/VkIgnite.hpp"

#include "glfw.hpp"

namespace vki::wsi::glfw {

[[nodiscard]] std::vector<ExtensionName> getRequiredExtensions();

[[nodiscard]] vk::UniqueSurfaceKHR createSurfaceKHRUnique(
    const vk::Instance& instance,
    GLFWwindow* window);

} // namespace vki::wsi::glfw
