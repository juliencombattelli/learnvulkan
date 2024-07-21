#pragma once

#include "spdlog.hpp"
#include "vulkan.hpp"

struct PhysicalDevicePickResult {
    vk::PhysicalDevice physicalDevice;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t presentationQueueFamilyIndex;
};

class PhysicalDevicePicker {
public:
    [[nodiscard]] static PhysicalDevicePickResult pick(
        const vk::Instance& instance,
        const vk::SurfaceKHR& surface,
        std::span<const std::string_view> requiredExtensions)
    {
        std::vector<PhysicalDevicePickResult> compatiblePhysicalDevices;

        const std::vector physicalDevices = instance.enumeratePhysicalDevices();
        for (const vk::PhysicalDevice& physicalDevice : physicalDevices) {
            std::optional<PhysicalDevicePickResult> physicalDevicePickResult
                = isPhysicalDeviceCompatible(physicalDevice, surface, requiredExtensions);
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
        std::span<const std::string_view> requiredExtensions)
    {
        const std::vector availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        bool extensionsSupported = true;
        for (std::string_view requiredExtension : requiredExtensions) {
            if (!contains(
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

    [[nodiscard]] static std::optional<uint32_t> findFirstGraphicsQueueIndex(
        const vk::PhysicalDevice& physicalDevice)
    {
        const std::vector queueFamiliesProperties = physicalDevice.getQueueFamilyProperties();
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamiliesProperties.size();
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

    [[nodiscard]] static std::optional<uint32_t> findFirstPresentationQueueIndex(
        const vk::PhysicalDevice& physicalDevice,
        const vk::SurfaceKHR& surface)
    {
        const std::vector queueFamiliesProperties = physicalDevice.getQueueFamilyProperties();
        for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamiliesProperties.size();
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
        std::span<const std::string_view> requiredExtensions)
    {
        std::string_view deviceName = physicalDevice.getProperties().deviceName;

        spdlog::debug("Checking if physical device {} is compatible", deviceName);

        bool requiredExtensionsAvailable
            = areRequiredDeviceExtensionsAvailable(physicalDevice, requiredExtensions);

        std::optional<uint32_t> graphicsQueueIndex = findFirstGraphicsQueueIndex(physicalDevice);

        std::optional<uint32_t> presentationQueueIndex
            = findFirstPresentationQueueIndex(physicalDevice, surface);

        std::initializer_list<bool> checks {
            requiredExtensionsAvailable,
            graphicsQueueIndex.has_value(),
            presentationQueueIndex.has_value(),
        };

        static constexpr auto fails = [](auto test) { return !test; };

        if (std::ranges::any_of(checks, fails)) {
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

    template<typename TContainer, typename TValue, typename TProjector = std::identity>
    [[nodiscard]] static bool contains(
        TContainer&& container,
        TValue&& value,
        TProjector projector = {})
    {
        return std::ranges::find(
                   std::forward<TContainer>(container),
                   std::forward<TValue>(value),
                   projector)
            != container.end();
    }
};
