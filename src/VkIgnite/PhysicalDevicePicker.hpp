#pragma once

#include "Stdx/Algorithm.hpp"

#include "VkIgnite.hpp"

#include "spdlog.hpp"
#include "vulkan.hpp"

namespace vki {

struct PhysicalDevicePickResult {
    vk::PhysicalDevice physicalDevice;
    QueueFamilyIndex graphicsQueueFamilyIndex;
    QueueFamilyIndex presentationQueueFamilyIndex;
};

// Pick a Vulkan physical device suitable for graphics rendering. If multiple
// devices are suitable, the preferred one is selected.
//
// Current suitability checks:
// - all required device extensions are available
// - the device provides a graphics queue
// - the device provides a presentation queue
//
// Device properties preference:
// - type: discrete > integrated > virtual > cpu > other
//
// TODO provide an environment variable to force the selection for the rare case
// when two or more GPUs with the same type is found (eg. two discrete graphics
// cards), or more generally for when the selection algorithm fails to select
// the more appropriate device.
class PhysicalDevicePicker {
public:
    [[nodiscard]] static PhysicalDevicePickResult pick(
        const vk::Instance& instance,
        const vk::SurfaceKHR& surface,
        std::span<vki::ExtensionName> requiredDeviceExtensions)
    {
        std::vector<PhysicalDevicePickResult> compatiblePhysicalDevices;

        const std::vector physicalDevices = instance.enumeratePhysicalDevices();
        for (const vk::PhysicalDevice& physicalDevice : physicalDevices) {
            std::optional<PhysicalDevicePickResult> physicalDevicePickResult
                = isPhysicalDeviceCompatible(physicalDevice, surface, requiredDeviceExtensions);
            if (physicalDevicePickResult.has_value()) {
                compatiblePhysicalDevices.push_back(*physicalDevicePickResult);
            }
        }

        if (compatiblePhysicalDevices.empty()) {
            std::runtime_error("No compatible physical device found");
        }

        std::ranges::sort(compatiblePhysicalDevices, compareDevicesByPreference);
        PhysicalDevicePickResult& preferedPhysicalDevices = compatiblePhysicalDevices.front();

        return preferedPhysicalDevices;
    }

private:
    [[nodiscard]] static bool compareDevicesByPreference(
        const PhysicalDevicePickResult& device1,
        const PhysicalDevicePickResult& device2)
    {
        const auto preferenceForDevice1
            = getDeviceTypePreference(device1.physicalDevice.getProperties().deviceType);
        const auto preferenceForDevice2
            = getDeviceTypePreference(device2.physicalDevice.getProperties().deviceType);
        return preferenceForDevice1 < preferenceForDevice2;
    }

    [[nodiscard]] static uint32_t getDeviceTypePreference(vk::PhysicalDeviceType type)
    {
        switch (type) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return 0;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return 1;
        case vk::PhysicalDeviceType::eVirtualGpu:
            return 2;
        case vk::PhysicalDeviceType::eCpu:
            return 3;
        case vk::PhysicalDeviceType::eOther:
            return 4;
        }
        throw std::logic_error("Unsupported vk::PhysicalDeviceType");
    }

    [[nodiscard]] static bool areRequiredDeviceExtensionsAvailable(
        const vk::PhysicalDevice& physicalDevice,
        std::span<vki::ExtensionName> requiredDeviceExtensions)
    {
        const std::vector availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        bool extensionsSupported = true;
        for (std::string_view requiredExtension : requiredDeviceExtensions) {
            if (!stdx::ranges::contains(
                    availableExtensions,
                    requiredExtension,
                    [](const vk::ExtensionProperties& extprop) -> std::string_view {
                        return extprop.extensionName;
                    })) {
                spdlog::debug("Physical device does not support extension {}", requiredExtension);
                extensionsSupported = false;
            }
        }
        return extensionsSupported;
    }

    [[nodiscard]] static std::optional<QueueFamilyIndex> findFirstGraphicsQueueIndex(
        const vk::PhysicalDevice& physicalDevice)
    {
        const std::vector queueFamiliesProperties = physicalDevice.getQueueFamilyProperties();
        const QueueFamilyIndex queueFamiliesCount { static_cast<uint32_t>(
            queueFamiliesProperties.size()) };
        for (QueueFamilyIndex queueFamilyIndex { 0u }; queueFamilyIndex < queueFamiliesCount;
             queueFamilyIndex++) {
            const vk::QueueFamilyProperties& queueFamilyProperty
                = queueFamiliesProperties[value_of(queueFamilyIndex)];
            if (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics) {
                return queueFamilyIndex;
            }
        }
        spdlog::debug("Incompatible physical device: no graphics queue");
        return std::nullopt;
    }

    [[nodiscard]] static std::optional<QueueFamilyIndex> findFirstPresentationQueueIndex(
        const vk::PhysicalDevice& physicalDevice,
        const vk::SurfaceKHR& surface)
    {
        const std::vector queueFamiliesProperties = physicalDevice.getQueueFamilyProperties();
        const QueueFamilyIndex queueFamiliesCount { static_cast<uint32_t>(
            queueFamiliesProperties.size()) };
        for (QueueFamilyIndex queueFamilyIndex { 0u }; queueFamilyIndex < queueFamiliesCount;
             queueFamilyIndex++) {
            if (physicalDevice.getSurfaceSupportKHR(value_of(queueFamilyIndex), surface)) {
                return queueFamilyIndex;
            }
        }
        spdlog::debug("Incompatible physical device: no presentation queue");
        return std::nullopt;
    }

    [[nodiscard]] static std::optional<PhysicalDevicePickResult> isPhysicalDeviceCompatible(
        const vk::PhysicalDevice& physicalDevice,
        const vk::SurfaceKHR& surface,
        std::span<vki::ExtensionName> requiredDeviceExtensions)
    {
        std::string_view deviceName = physicalDevice.getProperties().deviceName;

        spdlog::debug("Checking if physical device {} is compatible", deviceName);

        bool requiredExtensionsAvailable
            = areRequiredDeviceExtensionsAvailable(physicalDevice, requiredDeviceExtensions);

        std::optional<QueueFamilyIndex> graphicsQueueIndex
            = findFirstGraphicsQueueIndex(physicalDevice);

        std::optional<QueueFamilyIndex> presentationQueueIndex
            = findFirstPresentationQueueIndex(physicalDevice, surface);

        bool isCompatible = requiredExtensionsAvailable && graphicsQueueIndex.has_value()
            && presentationQueueIndex.has_value();

        if (!isCompatible) {
            spdlog::warn("Physical device {} is not compatible", deviceName);
            return std::nullopt;
        }

        spdlog::debug("Physical device {} is compatible", deviceName);

        return PhysicalDevicePickResult {
            .physicalDevice = physicalDevice,
            .graphicsQueueFamilyIndex = *graphicsQueueIndex,
            .presentationQueueFamilyIndex = *presentationQueueIndex,
        };
    }
};

} // namespace vki
