#pragma once

#include "vulkan.hpp"

#include <string>
#include <vector>

namespace vki {

using ExtensionName = const char*;
using LayerName = const char*;
using QueueFamilyIndex = uint32_t;

// A boolean value to control an option activation like extension or layer
enum class Option {
    Disabled,
    Enabled,
};

struct ApplicationInfo {
    std::string applicationName = {};
    uint32_t applicationVersion = {};
    std::string engineName = {};
    uint32_t engineVersion = {};
    uint32_t vkApiVersion = VK_MAKE_API_VERSION(0, 1, 3, 268);
};

struct InstanceCreateInfo {
    ApplicationInfo applicationInfo = {};
    // List of layers to enable
    std::vector<LayerName> enabledLayerNames = {};
    // List of extensions to enable
    std::vector<ExtensionName> enabledExtensionNames = {};
    // Whether to enable the validation layer from Khronos
    Option validationLayerKHROption = Option::Disabled;
    // Whether to enable the debug utils extension
    Option debugUtilsMessengerEXTOption = Option::Disabled;
};

[[nodiscard]] vk::UniqueInstance makeInstanceUnique(InstanceCreateInfo instanceCreateInfo);

[[nodiscard]] vk::UniqueDebugUtilsMessengerEXT makeDefaultDebugUtilsMessengerEXTUnique(
    vk::Instance instance);

struct QueueCreateInfo {
    vk::DeviceQueueCreateFlags flags = {};
    QueueFamilyIndex queueFamilyIndex = {};
    std::vector<float> queuePriorities = {};
};

struct DeviceCreateInfo {
    vk::DeviceCreateFlags flags = {};
    std::vector<QueueCreateInfo> queueCreateInfos = {};
    std::vector<LayerName> enabledLayerNames = {};
    std::vector<ExtensionName> enabledExtensionNames = {};
};

[[nodiscard]] vk::UniqueDevice makeDeviceUnique(
    vk::PhysicalDevice physicalDevice,
    DeviceCreateInfo deviceCreateInfo);

} // namespace vki
