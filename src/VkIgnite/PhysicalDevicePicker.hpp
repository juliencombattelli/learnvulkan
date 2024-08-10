#pragma once

#include "Stdx/Algorithm.hpp"

#include "VkIgnite.hpp"

#include "spdlog.hpp"
#include "vulkan.hpp"

#include <charconv>
#include <cstdlib>
#include <system_error>

namespace vki {

struct PhysicalDevicePickResult {
    vk::PhysicalDevice physicalDevice;
    QueueFamilyIndex graphicsQueueFamilyIndex;
    QueueFamilyIndex presentationQueueFamilyIndex;
    SwapchainSupportDetails swapchainSupportDetails;
};

// Pick a Vulkan physical device suitable for graphics rendering. If multiple
// devices are suitable, the preferred one is selected.
// If the DEVICE_ID environment variable is defined, the device having the
// corresponding ID is selected. If it is not found, the program terminates. To
// get the list of device IDs, run the program once without the environment
// variable and check the debug logs
//
// Current suitability checks:
// - all required device extensions are available
// - the device provides a graphics queue
// - the device provides a presentation queue
// - the surface provides at least one surface format
// - the surface provides at least one presentation mode
//
// Device properties preference:
// - type: discrete > integrated > virtual > cpu > other
class PhysicalDevicePicker {
public:
    [[nodiscard]] static PhysicalDevicePickResult pick(
        const vk::Instance& instance,
        const vk::SurfaceKHR& surface,
        std::span<vki::ExtensionName> requiredDeviceExtensions)
    {
        std::vector<PhysicalDevicePickResult> compatiblePhysicalDevices;

        spdlog::debug("Enumerating devices...");
        const std::vector physicalDevices = instance.enumeratePhysicalDevices();
        for (const vk::PhysicalDevice& physicalDevice : physicalDevices) {
            auto deviceProperties = physicalDevice.getProperties();
            spdlog::debug(
                "Device: ID={}, name=\"{}\"",
                deviceProperties.deviceID,
                std::string_view(deviceProperties.deviceName));
            std::optional<PhysicalDevicePickResult> physicalDevicePickResult
                = isPhysicalDeviceCompatible(physicalDevice, surface, requiredDeviceExtensions);
            if (physicalDevicePickResult.has_value()) {
                compatiblePhysicalDevices.push_back(*physicalDevicePickResult);
            }
        }

        if (compatiblePhysicalDevices.empty()) {
            std::runtime_error("No compatible physical device found");
        }

        char* userSelectedDeviceId = std::getenv("DEVICE_ID");
        if (userSelectedDeviceId != nullptr) {
            spdlog::debug("DEVICE_ID provided: {}", userSelectedDeviceId);
            return pickFromEnv(userSelectedDeviceId, compatiblePhysicalDevices);
        }

        std::ranges::sort(compatiblePhysicalDevices, compareDevicesByPreference);
        PhysicalDevicePickResult& preferedPhysicalDevice = compatiblePhysicalDevices.front();
        return preferedPhysicalDevice;
    }

private:
    [[nodiscard]] static PhysicalDevicePickResult pickFromEnv(
        std::string_view userSelectedDeviceId,
        const std::vector<PhysicalDevicePickResult>& compatiblePhysicalDevices)
    {

        uint32_t deviceId = 0;
        auto [ptr, ec] = std::from_chars(
            userSelectedDeviceId.data(),
            userSelectedDeviceId.data() + userSelectedDeviceId.size(),
            deviceId);
        if (ec != std::errc()) {
            switch (ec) {
            case std::errc::invalid_argument:
                spdlog::error("DEVICE_ID must be a valid uint32_t.");
                break;
            case std::errc::result_out_of_range:
                spdlog::error("DEVICE_ID is larger than an uint32_t.");
                break;
            default:
                spdlog::error("Unespected error while parsing DEVICE_ID.");
            }
            exit(EXIT_FAILURE);
        }
        auto selectedDeviceIt = std::ranges::find_if(
            compatiblePhysicalDevices,
            [deviceId](const PhysicalDevicePickResult& compatibleDevice) -> bool {
                return compatibleDevice.physicalDevice.getProperties().deviceID == deviceId;
            });
        if (selectedDeviceIt == compatiblePhysicalDevices.end()) {
            spdlog::error("Selected device ID {} not found.", deviceId);
            exit(EXIT_FAILURE);
        }
        return *selectedDeviceIt;
    }

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
        for (QueueFamilyIndex queueFamilyIndex = 0; queueFamilyIndex < queueFamiliesCount;
             queueFamilyIndex++) {
            const vk::QueueFamilyProperties& queueFamilyProperty
                = queueFamiliesProperties[queueFamilyIndex];
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
        for (QueueFamilyIndex queueFamilyIndex = 0; queueFamilyIndex < queueFamiliesCount;
             queueFamilyIndex++) {
            if (physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface)) {
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

        SwapchainSupportDetails swapchainSupport = querySwapchainSupport(physicalDevice, surface);
        bool swapchainAdequate
            = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();

        bool isCompatible = requiredExtensionsAvailable && graphicsQueueIndex.has_value()
            && presentationQueueIndex.has_value() && swapchainAdequate;

        if (!isCompatible) {
            spdlog::warn("Physical device {} is not compatible", deviceName);
            return std::nullopt;
        }

        spdlog::debug("Physical device {} is compatible", deviceName);

        return PhysicalDevicePickResult {
            .physicalDevice = physicalDevice,
            .graphicsQueueFamilyIndex = *graphicsQueueIndex,
            .presentationQueueFamilyIndex = *presentationQueueIndex,
            .swapchainSupportDetails = swapchainSupport,
        };
    }
};

} // namespace vki
