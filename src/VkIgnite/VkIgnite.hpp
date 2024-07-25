#pragma once

#include <strong_type/strong_type.hpp>

#include "vulkan.hpp"

#include <string>
#include <vector>

namespace vki {

using Version = strong::type<uint32_t, struct VersionTag, strong::default_constructible>;
using VkApiVersion = strong::type<uint32_t, struct VersionTag>;
using ExtensionName = const char*;
using LayerName = const char*;

using QueueFamilyIndex = strong::type<
    uint32_t,
    struct QueueFamilyIndexTag,
    strong::equality,
    strong::ordered,
    strong::bicrementable>;

[[nodiscard]] static inline constexpr Version makeVersion(
    uint32_t major,
    uint32_t minor,
    uint32_t patch) noexcept
{
    // Reuse the Vulkan version encoding for now, with a variant set to 0 to be
    // conform with semantic versionning.
    // The user can still use whatever encoding he wants for application and
    // engine versions.
    return Version { VK_MAKE_API_VERSION(0, major, minor, patch) };
}

[[nodiscard]] static inline constexpr VkApiVersion makeVkApiVersion(
    uint32_t variant,
    uint32_t major,
    uint32_t minor,
    uint32_t patch) noexcept
{
    return VkApiVersion { VK_MAKE_API_VERSION(variant, major, minor, patch) };
}

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
    VkApiVersion vkApiVersion = makeVkApiVersion(0, 1, 3, 268);
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
    QueueFamilyIndex queueFamilyIndex { strong::uninitialized };
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
