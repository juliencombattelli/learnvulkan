// Header files are included from a precompiled header generated by CMake

template<typename TContainer, typename TValue, typename TProjector = std::identity>
bool contains(TContainer&& container, TValue&& value, TProjector projector = {})
{
    return std::ranges::find(
               std::forward<TContainer>(container),
               std::forward<TValue>(value),
               projector)
        != container.end();
}

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

PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pMessenger)
{
    return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    VkAllocationCallbacks const* pAllocator)
{
    return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

class HelloTriangleApplication {
public:
    static inline constexpr uint32_t width = 800;
    static inline constexpr uint32_t height = 600;
    static inline constexpr bool enableValidationLayers = true;

    static inline const std::vector<const char*> requiredValidationLayers {
        "VK_LAYER_KHRONOS_validation",
    };

    static inline const std::vector<const char*> requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanupVulkan();
        cleanupWindow();
    }

private:
    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
    }

    [[nodiscard]] static std::vector<const char*> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    [[nodiscard]] static bool areValidationLayersSupported(std::vector<const char*> layers)
    {
        auto layerProperties = vk::enumerateInstanceLayerProperties();
        for (std::string_view layer : layers) {
            bool layerFound = false;
            for (vk::LayerProperties properties : layerProperties) {
                if (layer == properties.layerName) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static vk::Instance createVkInstance()
    {
        vk::ApplicationInfo appInfo {
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3,
        };

        auto extensions = getRequiredExtensions();

        vk::InstanceCreateInfo createInfo {
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        };

        if (enableValidationLayers) {
            if (!areValidationLayersSupported(requiredValidationLayers)) {
                throw std::runtime_error("Validation layers were requested but are not available");
            }
            createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
            createInfo.ppEnabledLayerNames = requiredValidationLayers.data();
        }

        return vk::createInstance(createInfo);
    }

    [[nodiscard]] static vk::DebugUtilsMessengerEXT createDebugMessenger(vk::Instance& instance)
    {
        pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
        pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

        using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
        using enum vk::DebugUtilsMessageTypeFlagBitsEXT;
        vk::DebugUtilsMessengerCreateInfoEXT createInfo {
            .messageSeverity = eVerbose | eInfo | eWarning | eError,
            .messageType = eGeneral | eValidation | ePerformance,
            .pfnUserCallback = debugCallback,
            .pUserData = nullptr,
        };
        return instance.createDebugUtilsMessengerEXT(createInfo);
    }

    [[nodiscard]] static bool areGraphicsAndPresentationCapabilitiesSupported(
        const vk::PhysicalDevice& device,
        const vk::SurfaceKHR& surface)
    {
        auto queueFamilyProperties = device.getQueueFamilyProperties();
        std::optional<uint32_t> queueGraphicsFamilyIndex;
        for (auto queueFamilyIt = queueFamilyProperties.begin();
             queueFamilyIt != queueFamilyProperties.end();
             ++queueFamilyIt) {
            if (queueFamilyIt->queueFlags & vk::QueueFlagBits::eGraphics) {
                queueGraphicsFamilyIndex
                    = std::distance(queueFamilyProperties.begin(), queueFamilyIt);
            }
            if (!queueGraphicsFamilyIndex) {
                spdlog::debug("Device does not support graphics");
                return false;
            }
            auto presentSupport = device.getSurfaceSupportKHR(*queueGraphicsFamilyIndex, surface);
            if (!presentSupport) {
                spdlog::debug("Device does not support presentation");
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] static bool areRequiredDeviceExtensionsAvailable(const vk::PhysicalDevice& device)
    {
        auto availableExtensions = device.enumerateDeviceExtensionProperties();
        bool extensionsSupported = true;
        // Check availability of all extensions
        for (std::string_view extension : requiredExtensions) {
            if (!contains(
                    availableExtensions,
                    extension,
                    [](auto const& extprop) -> std::string_view {
                        return extprop.extensionName;
                    })) {
                spdlog::debug("Device does not support extension {}", extension);
                extensionsSupported = false;
            }
        }
        return extensionsSupported;
    }

    [[nodiscard]] static bool isDeviceSuitable(const vk::PhysicalDevice& device)
    {
        return true;
    }

    [[nodiscard]] static vk::PhysicalDevice pickPhysicalDevice(vk::Instance& instance)
    {
        // TODO reuse device picker from Magma
        auto devices = instance.enumeratePhysicalDevices();
        auto selected_device = //
            std::ranges::find_if(devices, [](const auto& device) {
                return isDeviceSuitable(device);
            });
        if (selected_device == devices.end()) {
            throw std::runtime_error("No suitable GPU found");
        }
        return *selected_device;
    }

    void initVulkan()
    {
        instance = createVkInstance();
        if (enableValidationLayers) {
            debugMessenger = createDebugMessenger(instance);
        }
        physicalDevice = pickPhysicalDevice(instance);
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanupVulkan()
    {
        if (enableValidationLayers) {
            instance.destroyDebugUtilsMessengerEXT(debugMessenger);
        }
        instance.destroy();
    }

    void cleanupWindow()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow* window;
    vk::Instance instance;
    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::PhysicalDevice physicalDevice;
};

int main()
{
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
