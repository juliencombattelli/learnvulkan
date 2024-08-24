#pragma once

#include "Types.hpp"
#include "Version.hpp"

#include "Pch/Vulkan.hpp"

#include <strong_type/strong_type.hpp>

#include <string>
#include <vector>

namespace vki {

// A boolean value to control an option activation like extension or layer
enum class Option {
    Disabled,
    Enabled,
};

struct ApplicationInfo {
    std::string applicationName = {};
    Version applicationVersion = {};
    std::string engineName = {};
    Version engineVersion = {};
    ApiVersion vkApiVersion = ApiVersion::minimumRequired();
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

} // namespace vki
