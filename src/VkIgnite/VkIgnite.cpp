#include "VkIgnite.hpp"

namespace vki {

[[nodiscard]] vk::UniqueDevice makeDeviceUnique(
    vk::PhysicalDevice physicalDevice,
    DeviceCreateInfo deviceCreateInfo)
{
    // Prepare the creation of each desired device queues
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (QueueCreateInfo& queueCreateInfo : deviceCreateInfo.queueCreateInfos) {
        queueCreateInfos.push_back({
            .flags = queueCreateInfo.flags,
            .queueFamilyIndex = queueCreateInfo.queueFamilyIndex,
            .queueCount = static_cast<uint32_t>(queueCreateInfo.queuePriorities.size()),
            .pQueuePriorities = queueCreateInfo.queuePriorities.data(),
        });
    }

    // Create a logical device associated to the physical device
    return physicalDevice.createDeviceUnique({
        .flags = deviceCreateInfo.flags,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(deviceCreateInfo.enabledLayerNames.size()),
        .ppEnabledLayerNames = deviceCreateInfo.enabledLayerNames.data(),
        .enabledExtensionCount
        = static_cast<uint32_t>(deviceCreateInfo.enabledExtensionNames.size()),
        .ppEnabledExtensionNames = deviceCreateInfo.enabledExtensionNames.data(),
    });
}

[[nodiscard]] SwapchainSupportDetails querySwapchainSupport(
    const vk::PhysicalDevice& physicalDevice,
    const vk::SurfaceKHR& surface)
{
    return {
        .capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface),
        .formats = physicalDevice.getSurfaceFormatsKHR(surface),
        .presentModes = physicalDevice.getSurfacePresentModesKHR(surface),
    };
}

} // namespace vki
