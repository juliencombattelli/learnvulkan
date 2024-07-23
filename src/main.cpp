#include "VkIgnite/PhysicalDevicePicker.hpp"
#include "VkIgnite/VkIgnite.hpp"

#include "VkIgnite/Wsi/Glfw.hpp"

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

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

class HelloTriangleApplication {
public:
    static inline constexpr uint32_t width = 800;
    static inline constexpr uint32_t height = 600;
    static inline constexpr bool enableValidationLayers = true;

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

        window = glfwCreateWindow(width, height, "LearnVulkan", nullptr, nullptr);

        glfwSetKeyCallback(window, glfwKeyCallback);
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
            .enabledExtensions = vki::wsi::glfw::getRequiredExtensions(),
            .validationLayerKHROption = vki::Option::Enabled,
            .debugUtilsMessengerEXTOption = vki::Option::Enabled,
        });

        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

        if (enableValidationLayers) {
            debugMessenger = vki::makeDefaultDebugUtilsMessengerEXTUnique(*instance);
        }

        surface = vki::wsi::glfw::createSurfaceKHRUnique(*instance, window);

        std::vector<const std::string_view> requiredDeviceExtensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        PhysicalDevicePickResult physicalDevicePickResult
            = PhysicalDevicePicker::pick(*instance, *surface, requiredDeviceExtensions);

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
