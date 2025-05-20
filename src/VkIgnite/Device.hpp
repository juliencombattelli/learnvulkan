#pragma once

#include "Types.hpp"

#include "Pch/Vulkan.hpp"

namespace vki {

enum class LoadDeviceExtensions {
    No,
    Yes,
};

struct DefaultDispatchLoaderDynamicOptions {
    LoadDeviceExtensions loadDeviceExtensions;
};

struct QueueCreateInfo {
    vk::DeviceQueueCreateFlags flags = {};
    QueueFamilyIndex queueFamilyIndex = {};
    std::vector<float> queuePriorities = {};
};

struct DeviceCreateInfo {
    vk::DeviceCreateFlags flags = {};
    // List of queues to create per family
    std::vector<QueueCreateInfo> queueCreateInfos = {};
    // List of device layers to enable
    std::vector<LayerName> enabledLayerNames = {};
    // List of device extensions to enable
    std::vector<ExtensionName> enabledExtensionNames = {};
    // Options related to the default distpatch loader dynamic if used
    DefaultDispatchLoaderDynamicOptions dispatchLoaderOptions = {};
};

namespace ppdp {
// Polymorphic Physical Device Picker

}

class Device {
public:
    [[nodiscard]] static Device make(
        const DeviceCreateInfo& deviceCreateInfo,
        vk::PhysicalDevice physicalDevice);

    template<typename TPhysicalDevicePicker>
    [[nodiscard]] static Device make(
        const DeviceCreateInfo& deviceCreateInfo,
        const TPhysicalDevicePicker& physicalDevicePicker);

    vk::UniqueDevice handle;
};

} // namespace vki
