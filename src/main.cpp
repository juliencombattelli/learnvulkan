#include "PhysicalDevicePicker.hpp"

#include "glfw.hpp"

static void glfwErrorCallback(int errorCode, const char* description)
{
    spdlog::error("GLFW error {}: {}", errorCode, description);
}

static void glfwKeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

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

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

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
        glfwSetErrorCallback(glfwErrorCallback);

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);

        glfwSetKeyCallback(window, glfwKeyCallback);
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

    [[nodiscard]] static bool areValidationLayersSupported(std::span<const char* const> layers)
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

    [[nodiscard]] static vk::UniqueInstance createInstanceUnique()
    {
        vk::ApplicationInfo appInfo {
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
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

        return vk::createInstanceUnique(createInfo);
    }

    [[nodiscard]] static vk::UniqueDebugUtilsMessengerEXT createDebugMessengerUnique(
        vk::Instance& instance)
    {

        vk::DebugUtilsMessengerCreateInfoEXT createInfo {
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

        return instance.createDebugUtilsMessengerEXTUnique(createInfo);
    }

    [[nodiscard]] static vk::UniqueSurfaceKHR createSurfaceKHRUnique(
        const vk::Instance& instance,
        GLFWwindow* window)
    {
        VkSurfaceKHR surface;
        vk::Result err
            = static_cast<vk::Result>(glfwCreateWindowSurface(instance, window, nullptr, &surface));
        if (err != vk::Result::eSuccess) {
            throw std::runtime_error(vk::to_string(err));
        }
        return vk::UniqueSurfaceKHR { surface, instance };
    }

    void initVulkan()
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();
        instance = createInstanceUnique();
        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

        if (enableValidationLayers) {
            debugMessenger = createDebugMessengerUnique(*instance);
        }

        surface = createSurfaceKHRUnique(*instance, window);

        PhysicalDevicePickResult physicalDevicePickResult = PhysicalDevicePicker::pick(
            *instance,
            *surface,
            { { VK_KHR_SWAPCHAIN_EXTENSION_NAME } });

        physicalDevice = physicalDevicePickResult.physicalDevice;
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanupVulkan()
    {
    }

    void cleanupWindow()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow* window;
    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    vk::UniqueSurfaceKHR surface;
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
