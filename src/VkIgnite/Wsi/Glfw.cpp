#include "Glfw.hpp"

namespace vki::wsi::glfw {

[[nodiscard]] std::vector<ExtensionName> getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    ExtensionName* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<ExtensionName> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}

[[nodiscard]] vk::UniqueSurfaceKHR createSurfaceKHRUnique(
    const vk::Instance& instance,
    GLFWwindow* window)
{
    VkSurfaceKHR surface;
    vk::Result err
        = static_cast<vk::Result>(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    if (err != vk::Result::eSuccess) {
        throw std::runtime_error(vk::to_string(err));
    }
    return vk::UniqueSurfaceKHR { surface, instance };
}

} // namespace vki::wsi::glfw
