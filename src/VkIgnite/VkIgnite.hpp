#pragma once

#include "Instance.hpp"

#include "Pch/Vulkan.hpp"

#include <strong_type/strong_type.hpp>

#include <vector>

namespace vki {

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

struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

[[nodiscard]] SwapchainSupportDetails querySwapchainSupport(
    const vk::PhysicalDevice& physicalDevice,
    const vk::SurfaceKHR& surface);

} // namespace vki
