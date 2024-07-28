#pragma once

#include <strong_type/strong_type.hpp>

#include "vulkan.hpp"

#include <string>
#include <vector>

namespace vki {

using Version = strong::type<uint32_t, struct VersionTag, strong::default_constructible>;

// TODO move to defaults
[[nodiscard]] static inline constexpr Version makeVersion(
    uint32_t major,
    uint32_t minor,
    uint32_t patch) noexcept
{
    // Reuse the Vulkan version encoding for now, with a variant set to 0 to be
    // conform with semantic versionning.
    // The user can still use whatever encoding he wants for application and
    // engine versions.
    return Version { vk::makeApiVersion(0u, major, minor, patch) };
}

class ApiVersion {
public:
    [[nodiscard]] constexpr ApiVersion(
        uint32_t variant,
        uint32_t major,
        uint32_t minor,
        uint32_t patch)
        : value_ { vk::makeApiVersion(variant, major, minor, patch) } {};

    [[nodiscard]] constexpr uint32_t variant() const
    {
        return vk::apiVersionVariant(value_);
    }

    [[nodiscard]] constexpr uint32_t major() const
    {
        return vk::apiVersionMajor(value_);
    }

    [[nodiscard]] constexpr uint32_t minor() const
    {
        return vk::apiVersionMinor(value_);
    }

    [[nodiscard]] constexpr uint32_t patch() const
    {
        return vk::apiVersionPatch(value_);
    }

    [[nodiscard]] constexpr uint32_t value() const
    {
        return value_;
    }

private:
    uint32_t value_;
};

// Cannot use strong::type for those as they are given to Vulkan through arrays
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
    Version applicationVersion = {};
    std::string engineName = {};
    Version engineVersion = {};
    ApiVersion vkApiVersion { 0, 1, 3, 268 };
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
