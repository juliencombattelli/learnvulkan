#include "Device.hpp"

namespace vki {

[[nodiscard]] static vk::UniqueDevice makeDeviceUnique(
    const DeviceCreateInfo& deviceCreateInfo,
    vk::PhysicalDevice physicalDevice)
{
    // Prepare the creation of each desired device queues
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const QueueCreateInfo& queueCreateInfo : deviceCreateInfo.queueCreateInfos) {
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

[[nodiscard]] Device Device::make(
    const DeviceCreateInfo& deviceCreateInfo,
    vk::PhysicalDevice physicalDevice)
{
    vk::UniqueDevice device = vki::makeDeviceUnique(deviceCreateInfo, physicalDevice);

    { // Load device extension procedure addresses if requested by the user for this device
        using enum LoadDeviceExtensions;
        if (deviceCreateInfo.dispatchLoaderOptions.loadDeviceExtensions == Yes) {
            VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
        }
    }

    return {
        .handle = std::move(device),
    };
}

} // namespace vki
