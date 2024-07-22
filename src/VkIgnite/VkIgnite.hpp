#pragma once

#include "vulkan.hpp"

namespace vki {

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
    std::vector<const char*> enabledLayers = {};
    // List of extensions to enable
    std::vector<const char*> enabledExtensions = {};
    // Whether to enable the validation layer from Khronos
    Option validationLayerKHROption = Option::Disabled;
    // Whether to enable the debug utils extension
    Option debugUtilsMessengerEXTOption = Option::Disabled;
};

[[nodiscard]] vk::UniqueInstance makeInstanceUnique(InstanceCreateInfo instanceCreateInfo);

[[nodiscard]] vk::UniqueDebugUtilsMessengerEXT makeDefaultDebugUtilsMessengerEXTUnique(
    vk::Instance instance);

} // namespace vki
