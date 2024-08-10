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
    static inline constexpr uint32_t MaxFramesInFlight = 2;

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

        window_ = glfwCreateWindow(Width, Height, "LearnVulkan", nullptr, nullptr);

        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
        glfwSetKeyCallback(window_, glfwKeyCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/)
    {
        HelloTriangleApplication* app
            = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized_ = true;
    }

    void initVulkan()
    {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        instance_ = vki::makeInstanceUnique(vki::InstanceCreateInfo {
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

        VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

        if (EnableValidationLayers) {
            debugMessenger_ = vki::makeDefaultDebugUtilsMessengerEXTUnique(*instance_);
        }

        surface_ = vki::wsi::glfw::createSurfaceKHRUnique(*instance_, window_);

        std::vector<vki::ExtensionName> requiredDeviceExtensions {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        vki::PhysicalDevicePickResult physicalDevicePickResult
            = vki::PhysicalDevicePicker::pick(*instance_, *surface_, requiredDeviceExtensions);

        physicalDevice_ = physicalDevicePickResult.physicalDevice;

        // Save the index of both queue families
        queueFamiliesInfo_.graphicsQueueFamilyIndex
            = physicalDevicePickResult.graphicsQueueFamilyIndex;
        queueFamiliesInfo_.presentationQueueFamilyIndex
            = physicalDevicePickResult.presentationQueueFamilyIndex;

        // Create a list of queue family indices without duplicates
        queueFamiliesInfo_.queueFamilyIndices = {
            physicalDevicePickResult.graphicsQueueFamilyIndex,
            physicalDevicePickResult.presentationQueueFamilyIndex,
        };
        stdx::ranges::sort_unique(queueFamiliesInfo_.queueFamilyIndices);

        // Create one queue from each family with the same priority
        std::vector<vki::QueueCreateInfo> queueCreateInfos;
        for (auto& queueFamilyIndex : queueFamiliesInfo_.queueFamilyIndices) {
            queueCreateInfos.push_back({
                .queueFamilyIndex = queueFamilyIndex,
                .queuePriorities = { 1.f },
            });
        }

        // Create a logical device associated to the physical device
        device_ = vki::makeDeviceUnique(
            physicalDevice_,
            {
                .queueCreateInfos = queueCreateInfos,
                .enabledExtensionNames = requiredDeviceExtensions,
            });

        // Get the queue handles from the device
        graphicsQueue_ = device_->getQueue(physicalDevicePickResult.graphicsQueueFamilyIndex, 0);
        presentationQueue_
            = device_->getQueue(physicalDevicePickResult.presentationQueueFamilyIndex, 0);

        createSwapchain(physicalDevicePickResult.swapchainSupportDetails);

        createGraphicsPipeline();

        createCommandPool();
        createCommandBuffers();
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
        swapchainImageViews_.resize(swapchainImages_.size());
        for (size_t i = 0; i < swapchainImages_.size(); i++) {
            vk::ImageViewCreateInfo imageViewCreateInfo {
                .image = swapchainImages_[i],
                .viewType = vk::ImageViewType::e2D,
                .format = swapchainImageFormat_,
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
            swapchainImageViews_[i] = device_->createImageViewUnique(imageViewCreateInfo);
        }
    }

    void createSwapchain(const vki::SwapchainSupportDetails& swapchainSupport)
    {
        vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapchainSupport.formats);
        vk::PresentModeKHR presentMode = choosePresentMode(swapchainSupport.presentModes);
        vk::Extent2D extent = chooseExtent(window_, swapchainSupport.capabilities);
        uint32_t imageCount = chooseImageCount(swapchainSupport.capabilities);

        vk::SwapchainCreateInfoKHR swapchainCreateInfo {
            .flags = {},
            .surface = surface_.get(),
            .minImageCount = imageCount,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
            .imageSharingMode = vk::SharingMode::eExclusive,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = swapchainSupport.capabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = presentMode,
            .clipped = vk::True,
            .oldSwapchain = *swapchain_,
        };

        if (queueFamiliesInfo_.graphicsQueueFamilyIndex
            != queueFamiliesInfo_.presentationQueueFamilyIndex) {
            swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            swapchainCreateInfo.queueFamilyIndexCount
                = static_cast<uint32_t>(queueFamiliesInfo_.queueFamilyIndices.size());
            swapchainCreateInfo.pQueueFamilyIndices = queueFamiliesInfo_.queueFamilyIndices.data();
        }

        swapchain_ = device_->createSwapchainKHRUnique(swapchainCreateInfo);
        swapchainImages_ = device_->getSwapchainImagesKHR(*swapchain_);
        swapchainImageFormat_ = surfaceFormat.format;
        swapchainExtent_ = extent;

        createImageViews();
        createRenderPass();
        createFramebuffers();
    }

    void createRenderPass()
    {
        vk::AttachmentDescription colorAttachment {
            .format = swapchainImageFormat_,
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

        renderPass_ = device_->createRenderPassUnique(renderPassCreateInfo);
    }

    void createGraphicsPipeline()
    {
        shaderc::CompileOptions options;
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        vk::UniqueShaderModule vertexShader = vki::Shader::compileGlslToSpv(
            *device_,
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
            *device_,
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

        pipelineLayout_ = device_->createPipelineLayoutUnique(pipelineLayoutCreateInfo);

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
            .layout = *pipelineLayout_,
            .renderPass = *renderPass_,
            .subpass = 0,
            .basePipelineHandle = nullptr,
        };

        auto pipelineCreationResult = device_->createGraphicsPipelinesUnique(
            nullptr,
            { graphicsPipelineCreateInfo },
            nullptr);
        if (pipelineCreationResult.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }
        graphicsPipeline_ = std::move(pipelineCreationResult.value[0]);
    }

    void createFramebuffers()
    {
        framebuffers_.resize(swapchainImageViews_.size());

        for (size_t i = 0; i < swapchainImageViews_.size(); i++) {
            vk::FramebufferCreateInfo framebufferCreateInfo {
                .renderPass = *renderPass_,
                .attachmentCount = 1,
                .pAttachments = &*swapchainImageViews_[i],
                .width = swapchainExtent_.width,
                .height = swapchainExtent_.height,
                .layers = 1,
            };
            vk::UniqueFramebuffer framebuffer
                = device_->createFramebufferUnique(framebufferCreateInfo);
            if (!framebuffer) {
                throw std::runtime_error(std::format("Cannot create framebuffer #{}", i));
            }
            framebuffers_[i] = std::move(framebuffer);
        }
    }

    void createCommandPool()
    {
        vk::CommandPoolCreateInfo commandPoolCreateInfo {
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = queueFamiliesInfo_.graphicsQueueFamilyIndex,
        };
        commandPool_ = device_->createCommandPoolUnique(commandPoolCreateInfo);
    }

    void createCommandBuffers()
    {
        vk::CommandBufferAllocateInfo commandBufferAllocInfo {
            .commandPool = *commandPool_,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MaxFramesInFlight,
        };
        commandBuffers_ = device_->allocateCommandBuffersUnique(commandBufferAllocInfo);
    }

    void createSyncObjects()
    {
        vk::SemaphoreCreateInfo semaphoreCreateInfo {};

        vk::FenceCreateInfo fenceCreateInfo {
            .flags = vk::FenceCreateFlagBits::eSignaled,
        };

        for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
            imageAvailableSemaphores_.push_back(
                device_->createSemaphoreUnique(semaphoreCreateInfo));
            renderFinishedSemaphores_.push_back(
                device_->createSemaphoreUnique(semaphoreCreateInfo));
            inFlightFences_.push_back(device_->createFenceUnique(fenceCreateInfo));
        }
    }

    void recreateSwapchain()
    {
        framebufferResized_ = false;

        int width = 0, height = 0;
        glfwGetFramebufferSize(window_, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window_, &width, &height);
            glfwWaitEvents();
        }

        device_->waitIdle();

        vki::SwapchainSupportDetails swapchainSupportDetails
            = vki::querySwapchainSupport(physicalDevice_, *surface_);
        createSwapchain(swapchainSupportDetails);
    }

    void recordCommandBuffer(vk::CommandBuffer cmdBuffer, uint32_t imageIndex)
    {
        vk::CommandBufferBeginInfo commandBufferBeginInfo {};
        cmdBuffer.begin(commandBufferBeginInfo);

        vk::ClearValue clearColor { .color { .float32 { { 0.0f, 0.0f, 0.0f, 1.0f } } } };
        vk::RenderPassBeginInfo renderPassBeginInfo {
            .renderPass = *renderPass_,
            .framebuffer = *framebuffers_[imageIndex],
            .renderArea {
                .offset { .x = 0, .y = 0 },
                .extent = swapchainExtent_,
            },
            .clearValueCount = 1,
            .pClearValues = &clearColor,
        };

        cmdBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        {

            cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_);

            vk::Viewport viewport {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(swapchainExtent_.width),
                .height = static_cast<float>(swapchainExtent_.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            cmdBuffer.setViewport(0, { viewport });

            vk::Rect2D scissor {
                .offset { .x = 0, .y = 0 },
                .extent = swapchainExtent_,
            };
            cmdBuffer.setScissor(0, { scissor });

            cmdBuffer.draw(3, 1, 0, 0);
        }
        cmdBuffer.endRenderPass();

        cmdBuffer.end();
    }

    void drawFrame()
    {
        vk::Result waitResult = device_->waitForFences(
            { *inFlightFences_[currentFrame_] },
            vk::True,
            std::numeric_limits<uint64_t>::max());
        if (waitResult != vk::Result::eSuccess) {
            spdlog::warn("waitForFences returned {}, skipping frame", to_string(waitResult));
            return;
        }

        uint32_t imageIndex;
        vk::Result acquireNextImageResult = device_->acquireNextImageKHR(
            *swapchain_,
            std::numeric_limits<uint64_t>::max(),
            imageAvailableSemaphores_[currentFrame_].get(),
            {},
            &imageIndex);
        if (acquireNextImageResult == vk::Result::eErrorOutOfDateKHR) {
            spdlog::warn(
                "acquireNextImageKHR returned {}, recreating swapchain",
                to_string(acquireNextImageResult));
            recreateSwapchain();
            return;
        } else if (
            acquireNextImageResult != vk::Result::eSuccess
            && acquireNextImageResult != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        device_->resetFences({ *inFlightFences_[currentFrame_] });

        commandBuffers_[currentFrame_]->reset();

        recordCommandBuffer(*commandBuffers_[currentFrame_], imageIndex);

        vk::Semaphore waitSemaphores[] = { *imageAvailableSemaphores_[currentFrame_] };
        vk::Semaphore signalSemaphores[] = { *renderFinishedSemaphores_[currentFrame_] };
        vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

        vk::SubmitInfo submitInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = waitSemaphores,
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &*commandBuffers_[currentFrame_],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = signalSemaphores,
        };
        graphicsQueue_.submit({ submitInfo }, *inFlightFences_[currentFrame_]);

        vk::SwapchainKHR swapchains[] = { *swapchain_ };

        vk::PresentInfoKHR presentInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = signalSemaphores,
            .swapchainCount = 1,
            .pSwapchains = swapchains,
            .pImageIndices = &imageIndex,
        };
        vk::Result presentationResult = presentationQueue_.presentKHR(&presentInfo);
        if (presentationResult == vk::Result::eErrorOutOfDateKHR
            || presentationResult == vk::Result::eSuboptimalKHR || framebufferResized_) {
            spdlog::warn(
                "presentKHR returned {}, recreating swapchain",
                to_string(presentationResult));
            recreateSwapchain();
        } else if (presentationResult != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to present swapchain image!");
        }

        currentFrame_ = (currentFrame_ + 1) % MaxFramesInFlight;
    }

    void mainLoop()
    {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            drawFrame();
        }
        device_->waitIdle();
    }

    void cleanupVulkan()
    {
    }

    void cleanupWindow()
    {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    struct QueueFamiliesInfo {
        vki::QueueFamilyIndex graphicsQueueFamilyIndex;
        vki::QueueFamilyIndex presentationQueueFamilyIndex;
        std::vector<vki::QueueFamilyIndex> queueFamilyIndices;
    };

    GLFWwindow* window_;

    vk::UniqueInstance instance_;
    vk::UniqueDebugUtilsMessengerEXT debugMessenger_;

    vk::UniqueSurfaceKHR surface_;
    vk::PhysicalDevice physicalDevice_;
    vk::UniqueDevice device_;

    QueueFamiliesInfo queueFamiliesInfo_;
    vk::Queue graphicsQueue_;
    vk::Queue presentationQueue_;

    vk::UniqueSwapchainKHR swapchain_;
    std::vector<vk::Image> swapchainImages_;
    vk::Format swapchainImageFormat_;
    vk::Extent2D swapchainExtent_;
    std::vector<vk::UniqueImageView> swapchainImageViews_;
    std::vector<vk::UniqueFramebuffer> framebuffers_;

    vk::UniqueRenderPass renderPass_;
    vk::UniquePipelineLayout pipelineLayout_;
    vk::UniquePipeline graphicsPipeline_;

    vk::UniqueCommandPool commandPool_;
    std::vector<vk::UniqueCommandBuffer> commandBuffers_;

    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
    std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
    std::vector<vk::UniqueFence> inFlightFences_;
    uint32_t currentFrame_ = 0;

    bool framebufferResized_ = false;
};

int main()
{
    spdlog::set_level(spdlog::level::debug);

    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        spdlog::error("Caught unhandled exception!");
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
