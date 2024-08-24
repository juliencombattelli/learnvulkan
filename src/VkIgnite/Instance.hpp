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

using DebugUtilsMessengerCallback = PFN_vkDebugUtilsMessengerCallbackEXT;

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
    // Debug messenger callback, nullptr to use the engine's default one
    DebugUtilsMessengerCallback debugUtilsMessengerCallback = nullptr;
    // Debug messenger user data pointer, nullptr if not used
    void* debugUtilsMessengerUserData = nullptr;
};

class Instance {
public:
    [[nodiscard]] static Instance make(
        const InstanceCreateInfo& instanceCreateInfo,
        std::optional<vk::AllocationCallbacks> allocationCb = std::nullopt);

    vk::UniqueInstance instance;
    std::optional<vk::AllocationCallbacks> allocationCallbacks;
    vk::UniqueDebugUtilsMessengerEXT debugUtilsMessengerEXT;
};

} // namespace vki
