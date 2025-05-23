#include "Instance.hpp"

#include "Pch/Spdlog.hpp"
#include "Pch/Vulkan.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static bool defaultDebugCallbackCpp(
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

static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* cbData,
    void* userdata)
{
    vk::DebugUtilsMessengerCallbackDataEXT callbackData;
    callbackData = *cbData;
    return defaultDebugCallbackCpp(
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

[[nodiscard]] static bool isInstanceExtensionSupported(std::string_view extension)
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

[[nodiscard]] static vk::UniqueInstance makeInstanceUnique(InstanceCreateInfo instanceCreateInfo)
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
        && !isInstanceExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
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

[[nodiscard]] static vk::UniqueDebugUtilsMessengerEXT makeDebugUtilsMessengerEXTUnique(
    vk::Instance instance,
    std::optional<DebugUtilsMessengerCallback> debugUtilsMessengerCb)
{
    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = [&] {
        if (debugUtilsMessengerCb == std::nullopt) {
            // Use the default debug utils messenger if the user provided none
            return vk::DebugUtilsMessengerCreateInfoEXT {
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
                .pfnUserCallback = defaultDebugCallback,
                .pUserData = nullptr,
            };
        } else {
            return vk::DebugUtilsMessengerCreateInfoEXT {
                .messageSeverity = debugUtilsMessengerCb->messageSeverity,
                .messageType = debugUtilsMessengerCb->messageType,
                .pfnUserCallback = debugUtilsMessengerCb->callback,
                .pUserData = debugUtilsMessengerCb->userData,
            };
        }
    }();

    return instance.createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
}

[[nodiscard]] Instance Instance::make(const InstanceCreateInfo& instanceCreateInfo)
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    vk::UniqueInstance instance = vki::makeInstanceUnique(instanceCreateInfo);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    if (instanceCreateInfo.debugUtilsMessengerEXTOption == Option::Enabled) {
        debugMessenger = vki::makeDebugUtilsMessengerEXTUnique(
            *instance,
            instanceCreateInfo.debugUtilsMessengerCallback);
    }

    return {
        .handle = std::move(instance),
        .allocationCallbacks = instanceCreateInfo.allocationCallbacks,
        .debugUtilsMessengerEXT = std::move(debugMessenger),
    };
}

} // namespace vki
