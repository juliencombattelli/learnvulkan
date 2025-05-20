#pragma once

#include "Types.hpp"
#include "Version.hpp"

#include "Pch/Vulkan.hpp"

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

struct DebugUtilsMessengerCallback {
    vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity;
    vk::DebugUtilsMessageTypeFlagsEXT messageType;
    PFN_vkDebugUtilsMessengerCallbackEXT callback;
    void* userData;
};

struct InstanceCreateInfo {
    ApplicationInfo applicationInfo = {};
    // List of instance layers to enable
    std::vector<LayerName> enabledLayerNames = {};
    // List of instance extensions to enable
    std::vector<ExtensionName> enabledExtensionNames = {};
    // Whether to enable the validation layer from Khronos
    Option validationLayerKHROption = Option::Disabled;
    // Whether to enable the debug utils extension
    Option debugUtilsMessengerEXTOption = Option::Disabled;
    // Debug messenger callback, or nullopt to use the engine's default one
    std::optional<DebugUtilsMessengerCallback> debugUtilsMessengerCallback = std::nullopt;
    // Allocation callback, or nullopt if not used
    std::optional<vk::AllocationCallbacks> allocationCallbacks = std::nullopt;
};

class Instance {
public:
    [[nodiscard]] static Instance make(const InstanceCreateInfo& instanceCreateInfo);

    vk::UniqueInstance handle;
    std::optional<vk::AllocationCallbacks> allocationCallbacks;
    vk::UniqueDebugUtilsMessengerEXT debugUtilsMessengerEXT;
};

} // namespace vki
