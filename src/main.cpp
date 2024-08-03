#include "VkIgnite/PhysicalDevicePicker.hpp"
#include "VkIgnite/Shader.hpp"
#include "VkIgnite/VkIgnite.hpp"
#include "VkIgnite/Wsi/Glfw.hpp"

#include "spdlog/spdlog.h"

#include "Stdx/Algorithm.hpp"

#include <stdexcept>
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

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
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

        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffer();
        createSyncObjects();
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

    void createRenderPass()
    {
        vk::AttachmentDescription colorAttachment {
            .format = swapchainImageFormat,
            .samples = vk::SampleCountFlagBits::e1,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::ePresentSrcKHR,
        };

        vk::AttachmentReference colorAttachmentRef {
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
        };

        vk::SubpassDescription subpassDescription {
            .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef,
        };

        vk::SubpassDependency subpassDependency {
            .srcSubpass = vk::SubpassExternal,
            .dstSubpass = 0,
            .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        };

        vk::RenderPassCreateInfo renderPassCreateInfo {
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 1,
            .pDependencies = &subpassDependency,
        };

        renderPass = device->createRenderPassUnique(renderPassCreateInfo);
    }

    void createGraphicsPipeline()
    {
        shaderc::CompileOptions options;
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        vk::UniqueShaderModule vertexShader = vki::Shader::compileGlslToSpv(
            *device,
            kVertexShaderSource,
            vki::ShaderCompileInfo {
                .shaderKind = shaderc_vertex_shader,
                .inputIdentifier = "vertex shader",
                .option = options,
            });

        vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *vertexShader,
            .pName = "main",
        };

        vk::UniqueShaderModule fragmentShader = vki::Shader::compileGlslToSpv(
            *device,
            kFragmentShaderSource,
            vki::ShaderCompileInfo {
                .shaderKind = shaderc_fragment_shader,
                .inputIdentifier = "fragment shader",
                .option = options,
            });

        vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragmentShader,
            .pName = "main",
        };

        vk::PipelineShaderStageCreateInfo shaderStages[] = {
            vertexShaderStageCreateInfo,
            fragmentShaderStageCreateInfo,
        };

        std::vector<vk::DynamicState> dynamicStates {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        };
        vk::PipelineDynamicStateCreateInfo dynamicState {
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data(),
        };

        vk::PipelineVertexInputStateCreateInfo vertexInputState {
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = vk::False,
        };

        vk::PipelineViewportStateCreateInfo viewportState {
            .viewportCount = 1,
            .scissorCount = 1,
        };

        vk::PipelineRasterizationStateCreateInfo rasterizerState {
            .depthClampEnable = vk::False,
            .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eClockwise,
            .depthBiasEnable = vk::False,
            .lineWidth = 1.0f,
        };

        vk::PipelineMultisampleStateCreateInfo multisamplingState {
            .rasterizationSamples = vk::SampleCountFlagBits::e1,
            .sampleShadingEnable = vk::False,
        };

        using enum vk::ColorComponentFlagBits;
        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
            .blendEnable = vk::False,
            .colorWriteMask = eR | eG | eB | eA,
        };

        vk::PipelineColorBlendStateCreateInfo colorBlendingState {
            .logicOpEnable = vk::False,
            .logicOp = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachmentState,
            .blendConstants { { 0.0f, 0.0f, 0.0f, 0.0f } },
        };

        vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0,
        };

        pipelineLayout = device->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

        vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizerState,
            .pMultisampleState = &multisamplingState,
            .pColorBlendState = &colorBlendingState,
            .pDynamicState = &dynamicState,
            .layout = *pipelineLayout,
            .renderPass = *renderPass,
            .subpass = 0,
            .basePipelineHandle = nullptr,
        };

        auto pipelineCreationResult = device->createGraphicsPipelinesUnique(
            nullptr,
            { graphicsPipelineCreateInfo },
            nullptr);
        if (pipelineCreationResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }
        graphicsPipeline = std::move(pipelineCreationResult.value[0]);
    }

    void createFramebuffers()
    {
        framebuffers.resize(swapchainImageViews.size());

        for (size_t i = 0; i < swapchainImageViews.size(); i++) {
            vk::FramebufferCreateInfo framebufferCreateInfo {
                .renderPass = *renderPass,
                .attachmentCount = 1,
                .pAttachments = &*swapchainImageViews[i],
                .width = swapchainExtent.width,
                .height = swapchainExtent.height,
                .layers = 1,
            };
            vk::UniqueFramebuffer framebuffer
                = device->createFramebufferUnique(framebufferCreateInfo);
            if (!framebuffer) {
                throw std::runtime_error(std::format("Cannot create framebuffer #{}", i));
            }
            framebuffers[i] = std::move(framebuffer);
        }
    }

    void createCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamiliesInfo.graphicsQueueFamilyIndex,
        };
        commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);
    }

    void createCommandBuffer()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocInfo {
            .commandPool = *commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        commandBuffer = std::move(device->allocateCommandBuffersUnique(commandBufferAllocInfo)[0]);
    }

    void createSyncObjects()
    {
        vk::SemaphoreCreateInfo semaphoreCreateInfo {};
        imageAvailableSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);
        renderFinishedSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);

        vk::FenceCreateInfo fenceCreateInfo {
            .flags = vk::FenceCreateFlagBits::eSignaled,
        };
        inFlightFence = device->createFenceUnique(fenceCreateInfo);
    }

    void recordCommandBuffer(vk::CommandBuffer cmdBuffer, uint32_t imageIndex)
    {
        vk::CommandBufferBeginInfo commandBufferBeginInfo {};
        cmdBuffer.begin(commandBufferBeginInfo);

        vk::ClearValue clearColor { .color { .float32 { { 0.0f, 0.0f, 0.0f, 1.0f } } } };
        vk::RenderPassBeginInfo renderPassBeginInfo {
            .renderPass = *renderPass,
            .framebuffer = *framebuffers[imageIndex],
            .renderArea {
                .offset { .x = 0, .y = 0 },
                .extent = swapchainExtent,
            },
            .clearValueCount = 1,
            .pClearValues = &clearColor,
        };

        cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        {

            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

            vk::Viewport viewport {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(swapchainExtent.width),
                .height = static_cast<float>(swapchainExtent.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            cmdBuffer.setViewport(0, { viewport });

            vk::Rect2D scissor {
                .offset { .x = 0, .y = 0 },
                .extent = swapchainExtent,
            };
            cmdBuffer.setScissor(0, { scissor });

            cmdBuffer.draw(3, 1, 0, 0);
        }
        cmdBuffer.endRenderPass();

        cmdBuffer.end();
    }

    void drawFrame()
    {
        vk::Result waitResult = device->waitForFences(
            { *inFlightFence },
            vk::True,
            std::numeric_limits<uint64_t>::max());
        if (waitResult != vk::Result::eSuccess) {
            spdlog::warn("waitForFences returned {}, skipping frame", to_string(waitResult));
            return;
        }
        device->resetFences({ *inFlightFence });

        vk::ResultValue<uint32_t> imageIndex = device->acquireNextImageKHR(
            *swapchain,
            std::numeric_limits<uint64_t>::max(),
            imageAvailableSemaphore.get(),
            {});
        if (imageIndex.result != vk::Result::eSuccess) {
            spdlog::warn(
                "acquireNextImageKHR returned {}, skipping frame",
                to_string(imageIndex.result));
            return;
        }

        commandBuffer->reset();

        recordCommandBuffer(*commandBuffer, imageIndex.value);

        vk::Semaphore waitSemaphores[] = { *imageAvailableSemaphore };
        vk::Semaphore signalSemaphores[] = { *renderFinishedSemaphore };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

        vk::SubmitInfo submitInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores,
        };
        graphicsQueue.submit({ submitInfo }, *inFlightFence);

        vk::SwapchainKHR swapchains[] = { *swapchain };

        vk::PresentInfoKHR presentInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapchains,
            .pImageIndices = &imageIndex.value,
        };
        vk::Result presentationResult = presentationQueue.presentKHR(presentInfo);
        if (presentationResult != vk::Result::eSuccess) {
            spdlog::warn("presentKHR returned ", to_string(presentationResult));
        }
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
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
    std::vector<vk::UniqueFramebuffer> framebuffers;

    vk::UniqueRenderPass renderPass;
    vk::UniquePipelineLayout pipelineLayout;
    vk::UniquePipeline graphicsPipeline;

    vk::UniqueCommandPool commandPool;
    vk::UniqueCommandBuffer commandBuffer;

    vk::UniqueSemaphore imageAvailableSemaphore;
    vk::UniqueSemaphore renderFinishedSemaphore;
    vk::UniqueFence inFlightFence;
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
