#include "VkIgnite/PhysicalDevicePicker.hpp"
#include "VkIgnite/VkIgnite.hpp"

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

        instance = vki::makeInstanceUnique(vki::InstanceCreateInfo {
            .applicationInfo = {
                .applicationName = "",
                .applicationVersion = 0,
                .engineName = "",
                .engineVersion = 0,
            },
            .enabledLayers = {},
            .enabledExtensions = getRequiredExtensions(),
            .validationLayerKHROption = vki::Option::Enabled,
            .debugUtilsMessengerEXTOption = vki::Option::Enabled,
        });

        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

        if (enableValidationLayers) {
            debugMessenger = vki::makeDefaultDebugUtilsMessengerEXTUnique(*instance);
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
    spdlog::set_level(spdlog::level::debug);

    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
