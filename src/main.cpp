#include "VkIgnite/PhysicalDevicePicker.hpp"
#include "VkIgnite/Shader.hpp"
#include "VkIgnite/VkIgnite.hpp"
#include "VkIgnite/Wsi/Glfw.hpp"

#include "spdlog/spdlog.h"

#include "Stdx/Algorithm.hpp"

#include <vector>

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

constexpr const char kVertexShaderSource[] = R"vertexshader(
#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 0) out vec3 fragColor;
void main() {
    gl_Position = vec4(position, 1.0);
    fragColor = color;
}
)vertexshader";

constexpr const char kFragmentShaderSource[] = R"fragmentShader(
#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(fragColor, 1.0);
}
)fragmentShader";

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

class HelloTriangleApplication {
public:
    static inline constexpr uint32_t Width = 800;
    static inline constexpr uint32_t Height = 600;
    static inline constexpr bool EnableValidationLayers = true;

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

        window = glfwCreateWindow(Width, Height, "LearnVulkan", nullptr, nullptr);

        glfwSetKeyCallback(window, glfwKeyCallback);
    }

    void initVulkan()
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        instance = vki::makeInstanceUnique(vki::InstanceCreateInfo {
            .applicationInfo = {
                .applicationName = "",
                .applicationVersion = vki::makeVersion(0, 1, 0),
                .engineName = "",
                .engineVersion = vki::makeVersion(0, 1, 0),
            },
            .enabledLayerNames = {},
            .enabledExtensionNames = vki::wsi::glfw::getRequiredExtensions(),
            .validationLayerKHROption = vki::Option::Enabled,
            .debugUtilsMessengerEXTOption = vki::Option::Enabled,
        });

        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

        if (EnableValidationLayers) {
            debugMessenger = vki::makeDefaultDebugUtilsMessengerEXTUnique(*instance);
        }

        surface = vki::wsi::glfw::createSurfaceKHRUnique(*instance, window);

        std::vector<vki::ExtensionName> requiredDeviceExtensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vki::PhysicalDevicePickResult physicalDevicePickResult
            = vki::PhysicalDevicePicker::pick(*instance, *surface, requiredDeviceExtensions);

        physicalDevice = physicalDevicePickResult.physicalDevice;

        // Save the index of both queue families
        queueFamiliesInfo.graphicsQueueFamilyIndex
            = physicalDevicePickResult.graphicsQueueFamilyIndex;
        queueFamiliesInfo.presentationQueueFamilyIndex
            = physicalDevicePickResult.presentationQueueFamilyIndex;

        // Create a list of queue family indices without duplicates
        queueFamiliesInfo.queueFamilyIndices = {
            physicalDevicePickResult.graphicsQueueFamilyIndex,
            physicalDevicePickResult.presentationQueueFamilyIndex,
        };
        stdx::ranges::sort_unique(queueFamiliesInfo.queueFamilyIndices);

        // Create one queue from each family with the same priority
        std::vector<vki::QueueCreateInfo> queueCreateInfos;
        for (auto& queueFamilyIndex : queueFamiliesInfo.queueFamilyIndices) {
            queueCreateInfos.push_back({
                .queueFamilyIndex = queueFamilyIndex,
                .queuePriorities = { 1.f },
            });
        }

        // Create a logical device associated to the physical device
        device = vki::makeDeviceUnique(
            physicalDevice,
            {
                .queueCreateInfos = queueCreateInfos,
                .enabledExtensionNames = requiredDeviceExtensions,
            });

        // Get the queue handles from the device
        graphicsQueue = device->getQueue(physicalDevicePickResult.graphicsQueueFamilyIndex, 0);
        presentationQueue
            = device->getQueue(physicalDevicePickResult.presentationQueueFamilyIndex, 0);

        createSwapchain(physicalDevicePickResult.swapchainSupportDetails);

        compileShaders();
    }

    [[nodiscard]] static vk::SurfaceFormatKHR chooseSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
        // Prefer sRGB color space as it results in more accurate perceived colors
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == vk::Format::eR8G8B8A8Srgb
                && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }
        // Otherwise return the first one available
        return availableFormats[0];
    }

    [[nodiscard]] static vk::PresentModeKHR choosePresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
        // Prefer Mailbox (triple buffering V-Sync) to avoid tearing and reduce latency
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        // Otherwise fallback to Fifo (double buffering V-Sync) which is garanteed to be available
        return vk::PresentModeKHR::eFifo;
    }

    [[nodiscard]] static vk::Extent2D chooseExtent(
        GLFWwindow* window,
        const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            return {
                .width = std::clamp(
                    static_cast<uint32_t>(width),
                    capabilities.minImageExtent.width,
                    capabilities.maxImageExtent.width),
                .height = std::clamp(
                    static_cast<uint32_t>(height),
                    capabilities.minImageExtent.height,
                    capabilities.maxImageExtent.height),
            };
        }
    }

    [[nodiscard]] static uint32_t chooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities)
    {
        // Allocate one more image than the strict minimum the driver requires to work properly to
        // avoid waiting on the driver
        uint32_t imageCount = capabilities.minImageCount + 1;
        // Ensure the maximum number of image supported by the driver is not exceeded
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }
        spdlog::debug("Minimal image count: {}", imageCount);
        return imageCount;
    }

    void createImageViews()
    {
        swapchainImageViews.resize(swapchainImages.size());
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            vk::ImageViewCreateInfo imageViewCreateInfo {
                .image = swapchainImages[i],
                .viewType = vk::ImageViewType::e2D,
                .format = swapchainImageFormat,
                .components= {
                    .r = vk::ComponentSwizzle::eIdentity,
                    .g = vk::ComponentSwizzle::eIdentity,
                    .b = vk::ComponentSwizzle::eIdentity,
                    .a = vk::ComponentSwizzle::eIdentity,
                },
                .subresourceRange = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };
            swapchainImageViews[i] = device->createImageViewUnique(imageViewCreateInfo);
        }
    }

    void createSwapchain(const vki::SwapchainSupportDetails& swapchainSupportDetails)
    {
        vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapchainSupportDetails.formats);
        vk::PresentModeKHR presentMode = choosePresentMode(swapchainSupportDetails.presentModes);
        vk::Extent2D extent = chooseExtent(window, swapchainSupportDetails.capabilities);
        uint32_t imageCount = chooseImageCount(swapchainSupportDetails.capabilities);

        vk::SwapchainCreateInfoKHR swapchainCreateInfo {
            .flags = {},
            .surface = surface.get(),
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = swapchainSupportDetails.capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = presentMode,
            .clipped = vk::True,
            .oldSwapchain = nullptr,
        };

        if (queueFamiliesInfo.graphicsQueueFamilyIndex
            != queueFamiliesInfo.presentationQueueFamilyIndex) {
            swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            swapchainCreateInfo.queueFamilyIndexCount
                = static_cast<uint32_t>(queueFamiliesInfo.queueFamilyIndices.size());
            swapchainCreateInfo.pQueueFamilyIndices = queueFamiliesInfo.queueFamilyIndices.data();
        }

        swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);
        swapchainImages = device->getSwapchainImagesKHR(*swapchain);
        swapchainImageFormat = surfaceFormat.format;
        swapchainExtent = extent;

        createImageViews();
    }

    void compileShaders()
    {
        shaderc::CompileOptions options;
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        vertexShader = vki::Shader::compileGlslToSpv(
            *device,
            kVertexShaderSource,
            vki::ShaderCompileInfo {
                .shaderKind = shaderc_vertex_shader,
                .inputIdentifier = "vertex shader",
                .entryPointName = "main",
                .option = options,
            });

        fragmentShader = vki::Shader::compileGlslToSpv(
            *device,
            kFragmentShaderSource,
            vki::ShaderCompileInfo {
                .shaderKind = shaderc_fragment_shader,
                .inputIdentifier = "fragment shader",
                .entryPointName = "main",
                .option = options,
            });
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

    struct QueueFamiliesInfo {
        vki::QueueFamilyIndex graphicsQueueFamilyIndex;
        vki::QueueFamilyIndex presentationQueueFamilyIndex;
        std::vector<vki::QueueFamilyIndex> queueFamilyIndices;
    };

    GLFWwindow* window;
    vk::UniqueInstance instance;
    vk::UniqueDebugUtilsMessengerEXT debugMessenger;
    vk::UniqueSurfaceKHR surface;
    vk::PhysicalDevice physicalDevice;
    vk::UniqueDevice device;
    QueueFamiliesInfo queueFamiliesInfo;
    vk::Queue graphicsQueue;
    vk::Queue presentationQueue;
    vk::UniqueSwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    vk::Format swapchainImageFormat;
    vk::Extent2D swapchainExtent;
    std::vector<vk::UniqueImageView> swapchainImageViews;
    vk::UniqueShaderModule vertexShader;
    vk::UniqueShaderModule fragmentShader;
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
