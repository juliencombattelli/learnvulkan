#include "VkIgnite.hpp"

#include "spdlog.hpp"

static bool debugCallbackCpp(
    vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    vk::DebugUtilsMessageTypeFlagsEXT /*type*/,
    const vk::DebugUtilsMessengerCallbackDataEXT& cbData,
    void* /*userdata*/)
{
    using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
    if /*  */ (severity & eError) {
        spdlog::error(cbData.pMessage);
    } else if (severity & eWarning) {
        spdlog::warn(cbData.pMessage);
    } else if (severity & eInfo) {
        spdlog::info(cbData.pMessage);
    } else if (severity & eVerbose) {
        spdlog::debug(cbData.pMessage);
    }
    return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* cbData,
    void* userdata)
{
    vk::DebugUtilsMessengerCallbackDataEXT callbackData;
    callbackData = *cbData;
    return debugCallbackCpp(
               static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity),
               static_cast<vk::DebugUtilsMessageTypeFlagBitsEXT>(type),
               callbackData,
               userdata)
        ? VK_TRUE
        : VK_FALSE;
}

namespace vki {

[[nodiscard]] static bool isValidationLayerSupported(std::string_view layer)
{
    std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();
    for (const vk::LayerProperties& properties : layerProperties) {
        if (layer == properties.layerName) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] static bool isExtensionSupported(std::string_view extension)
{
    std::vector<vk::ExtensionProperties> extensionProperties
        = vk::enumerateInstanceExtensionProperties();
    for (const vk::ExtensionProperties& properties : extensionProperties) {
        if (extension == properties.extensionName) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] vk::UniqueInstance makeInstanceUnique(InstanceCreateInfo instanceCreateInfo)
{
    vk::ApplicationInfo appInfo {
        .pApplicationName = instanceCreateInfo.applicationInfo.applicationName.c_str(),
        .applicationVersion = value_of(instanceCreateInfo.applicationInfo.applicationVersion),
        .pEngineName = instanceCreateInfo.applicationInfo.engineName.c_str(),
        .engineVersion = value_of(instanceCreateInfo.applicationInfo.engineVersion),
        .apiVersion = instanceCreateInfo.applicationInfo.vkApiVersion.value(),
    };

    // Update the debug utils extension requirement according to validation layer requirement
    if (instanceCreateInfo.validationLayerKHROption == Option::Enabled) {
        instanceCreateInfo.debugUtilsMessengerEXTOption = Option::Enabled;
    }

    // Ensure the debug utils extension is present if required
    if (instanceCreateInfo.debugUtilsMessengerEXTOption == Option::Enabled
        && !isExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        throw std::runtime_error(VK_EXT_DEBUG_UTILS_EXTENSION_NAME " is not available");
    }

    // Add the debug utils extension to the list if needed
    if (instanceCreateInfo.debugUtilsMessengerEXTOption == Option::Enabled) {
        instanceCreateInfo.enabledExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Ensure the validation layer is present if required
    if (instanceCreateInfo.validationLayerKHROption == Option::Enabled
        && !isValidationLayerSupported("VK_LAYER_KHRONOS_validation")) {
        throw std::runtime_error("VK_LAYER_KHRONOS_validation is not available");
    }

    // Add the validation layer to the list if needed
    if (instanceCreateInfo.validationLayerKHROption == Option::Enabled) {
        instanceCreateInfo.enabledLayerNames.push_back("VK_LAYER_KHRONOS_validation");
    }

    return vk::createInstanceUnique({
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(instanceCreateInfo.enabledLayerNames.size()),
        .ppEnabledLayerNames = instanceCreateInfo.enabledLayerNames.data(),
        .enabledExtensionCount
        = static_cast<uint32_t>(instanceCreateInfo.enabledExtensionNames.size()),
        .ppEnabledExtensionNames = instanceCreateInfo.enabledExtensionNames.data(),
    });
}

[[nodiscard]] vk::UniqueDebugUtilsMessengerEXT makeDefaultDebugUtilsMessengerEXTUnique(
    vk::Instance instance)
{
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
        .messageSeverity =
            []() {
                using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
                return eVerbose | eInfo | eWarning | eError;
            }(),
        .messageType =
            []() {
                using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
                return eGeneral | eValidation | ePerformance;
            }(),
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr,
    };

    return instance.createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
}

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
