// Based on https://github.com/dokipen3d/vulkanHppMinimalExample

#include "glfw.hpp"
#include "glm.hpp"
#include "vulkan.hpp"

#include <shaderc/shaderc.hpp>

#include <algorithm>
#include <array>
#include <bitset>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <vector>

template<typename... TArgs>
constexpr void unused(TArgs... args) noexcept
{
    (static_cast<void>(args), ...);
}

constexpr uint32_t width = 640;
constexpr uint32_t height = 480;

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

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};

struct VertexInputDescription {
    static constexpr inline std::array<vk::VertexInputBindingDescription, 1> bindings {
        vk::VertexInputBindingDescription {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex,
        },
    };
    static constexpr inline std::array<vk::VertexInputAttributeDescription, 2> attributes {
        vk::VertexInputAttributeDescription {
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, position),
        },
        vk::VertexInputAttributeDescription {
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color),
        },
    };
};

const std::array vertices {
    Vertex { .position = { 0.0f, -0.5f, 0.0f }, .color = { 1.0f, 0.0f, 0.0f } },
    Vertex { .position = { 0.5f, 0.5f, 0.0f }, .color = { 0.0f, 1.0f, 0.0f } },
    Vertex { .position = { -0.5f, 0.5f, 0.0f }, .color = { 0.0f, 0.0f, 1.0f } },
};

uint32_t findMemoryType(
    const vk::PhysicalDeviceMemoryProperties& memoryProperties,
    uint32_t typeBits,
    vk::MemoryPropertyFlags requirementsMask)
{
    auto typeIndex = uint32_t(~0);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1)
            && ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask)
                == requirementsMask)) {
            typeIndex = i;
            break;
        }
        typeBits >>= 1;
    }
    if (typeIndex == uint32_t(~0)) {
        throw std::runtime_error("No valid memory type found");
    }
    return typeIndex;
}

namespace utils {

template<typename TContainer, typename TValue, typename TProjector = std::identity>
bool contains(const TContainer& container, const TValue& value, TProjector projector = {})
{
    return std::ranges::find(container, value, projector) != container.end();
}

} // namespace utils

class DefaultPhysicalDevicePicker {
public:
    const vk::PhysicalDevice& operator()(const std::vector<vk::PhysicalDevice>& devices) const
    {
        return pick(devices);
    }

private:
    using Score = int;

    static const vk::PhysicalDevice& pick(const std::vector<vk::PhysicalDevice>& devices)
    {
        pickFromEnv(devices);

        std::vector<int> scores;
        scores.reserve(devices.size());
        for (const auto& device : devices) {
            std::string deviceName = device.getProperties().deviceName;

            auto memoryScore = getDeviceMemoryScore(device.getMemoryProperties());
            std::cout << "Device " << deviceName << " got a memoryScore of " << memoryScore << "\n";

            auto typeScore = getDeviceTypeScore(device.getProperties().deviceType);
            std::cout << "Device " << deviceName << " got a typeScore of " << typeScore << "\n";

            scores.push_back(memoryScore + typeScore);
            std::cout << "Device " << deviceName << " got " << scores.back() << "\n";
        }
        auto bestDeviceIndex = std::distance(scores.begin(), std::ranges::max_element(scores));
        return devices.at(bestDeviceIndex);
    }

    static void pickFromEnv(const std::vector<vk::PhysicalDevice>& devices)
    {
        const char* deviceNameEnvVar = std::getenv("MAGMA_DEVICE_NAME");
        std::string selectedDeviceName = deviceNameEnvVar ? deviceNameEnvVar : "";
        std::cout << "MAGMA_DEVICE_NAME=" << selectedDeviceName << "\n";
        // TODO override device selection
    }

    static Score getDeviceTypeScore(vk::PhysicalDeviceType type)
    {
        switch (type) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return 1 << 16;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return 1 << 8;
        case vk::PhysicalDeviceType::eVirtualGpu:
        case vk::PhysicalDeviceType::eCpu:
        case vk::PhysicalDeviceType::eOther:
            return 0;
        }
        throw std::logic_error("Unsupported vk::PhysicalDeviceType");
    }

    static Score getDeviceMemoryScore(const vk::PhysicalDeviceMemoryProperties& memoryProperties)
    {
        for (const auto& heap : memoryProperties.memoryHeaps) {
            std::cout << "type: " << std::bitset<8>((VkMemoryHeapFlags)heap.flags)
                      << ", size: " << heap.size << "\n";
        }
        constexpr auto isLocalHeap = [](const vk::MemoryHeap& heap) {
            return bool(heap.flags & vk::MemoryHeapFlagBits::eDeviceLocal);
        };
        auto localHeaps
            = std::span(memoryProperties.memoryHeaps.data(), memoryProperties.memoryHeapCount)
            | std::views::filter(isLocalHeap);
        auto largestHeap = std::ranges::max(localHeaps, {}, &vk::MemoryHeap::size);
        // Score is number of GB of memory
        return Score(largestHeap.size / (1024 * 1024 * 1024));
    }
};

static bool hasDeviceExtensions(
    const vk::PhysicalDevice& device,
    std::span<const std::string_view> extensions)
{
    const auto availableExtensions = device.enumerateDeviceExtensionProperties();
    bool extensionsSupported = true;
    for (const auto extension : extensions) {
        if (!utils::contains(
                availableExtensions,
                extension,
                [](auto const& extprop) -> std::string_view { return extprop.extensionName; })) {
            std::cout << "Device does not support extension " << extension << "\n";
            extensionsSupported = false;
        }
    }
    return extensionsSupported;
}

static bool hasQueueCapabilities(
    const vk::PhysicalDevice& device,
    vk::QueueFlags queueCapabilities,
    const vk::SurfaceKHR& surface)
{
    // TODO check presentation support
    // auto presentSupport = device.getSurfaceSupportKHR(*queueGraphicsFamilyIndex, surface);
    const auto queueFamilyProperties = device.getQueueFamilyProperties();
    return std::ranges::find_if(
               queueFamilyProperties,
               [queueCapabilities](const auto& properties) {
                   return (properties.queueFlags & queueCapabilities) == queueCapabilities
                       && properties.queueCount > 0;
               })
        != std::end(queueFamilyProperties);
}

struct PickPhysicalDeviceInfo {
    vk::SurfaceKHR surface;
    std::span<const std::string_view> requiredExtensions;
    vk::QueueFlags requiredQueueCapabilities;
};

bool isDeviceCompatible(const vk::PhysicalDevice& device, const PickPhysicalDeviceInfo& pickInfo)
{
    const std::string deviceName = device.getProperties().deviceName;

    std::cout << "Checking if physical device " << deviceName << " is compatible\n";

    constexpr auto fails = std::logical_not<bool> {};
    auto checks = {
        hasDeviceExtensions(device, pickInfo.requiredExtensions),
        hasQueueCapabilities(device, pickInfo.requiredQueueCapabilities, pickInfo.surface),
    };
    if (std::ranges::any_of(checks, fails)) {
        std::cout << "Physical device " << deviceName << " is not compatible\n";
        return false;
    }

    std::cout << "Physical device " << deviceName << " is compatible\n";
    return true;
}

void removeIncompatiblePhysicalDevices(
    std::vector<vk::PhysicalDevice>& devices,
    const PickPhysicalDeviceInfo& pickInfo)
{
    auto incompatibleCount = std::erase_if(devices, [&pickInfo](auto&& device) {
        return !isDeviceCompatible(device, pickInfo);
    });
    std::cout << "Removed " << incompatibleCount << " incompatible physical devices\n";
}

template<typename TPhysicalDevicePicker = DefaultPhysicalDevicePicker>
vk::PhysicalDevice pickDevice(
    const vk::Instance& instance,
    const PickPhysicalDeviceInfo& pickInfo,
    TPhysicalDevicePicker pick = {})
{
    auto devices = instance.enumeratePhysicalDevices();
    removeIncompatiblePhysicalDevices(devices, pickInfo);
    auto device = pick(devices);
    std::cout << "Selected " << device.getProperties().deviceName << "\n";
    return device;
}

static void key_callback(
    GLFWwindow* window,
    int key,
    [[maybe_unused]] int scancode,
    int action,
    [[maybe_unused]] int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    [[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void run(GLFWwindow* window)
{
    // Set application info
    vk::ApplicationInfo appInfo {
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_2,
    };

    // Set image format and size
    constexpr vk::Format format = vk::Format::eB8G8R8A8Unorm;
    constexpr vk::Extent2D extent { .width = width, .height = height };

    // Set image count for the swapchain (V-Sync)
    // Use 2 for double buffering
    // Use 3 for triple buffering
    constexpr uint32_t imageCount = 3;

    // Multisamples anti-aliasing
    // TODO support sample count greater than 1
    constexpr vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

    // Set a list of wanted extensions
    uint32_t glfwExtensionCount = 0;
    // Get GLFW's Window System Integration extensions
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back("VK_EXT_debug_utils");

    // Set a list wanted layers
    std::vector<const char*> layers { "VK_LAYER_KHRONOS_validation" };

    // Create Vulkan instance
    vk::UniqueInstance instance = vk::createInstanceUnique({
        .flags = {},
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    });

    // Create dynamic loader
    vk::DispatchLoaderDynamic dldi(*instance, vkGetInstanceProcAddr);

    // Create messenger for debugging
    auto messenger = instance->createDebugUtilsMessengerEXTUnique(
        {
            .flags = {},
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = debugCallback,
            .pUserData = nullptr,
        },
        nullptr,
        dldi);

    // Create window surface
    const auto createWindowSurface = [&] {
        VkSurfaceKHR surface;
        vk::Result err = static_cast<vk::Result>(
            glfwCreateWindowSurface(*instance, window, nullptr, &surface));
        if (err != vk::Result::eSuccess) {
            throw std::runtime_error(vk::to_string(err));
        }
        return surface;
    };
    vk::UniqueSurfaceKHR surface(createWindowSurface(), *instance);

    // Get all physical devices and print their properties
    std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();
    for (auto& d : physicalDevices) {
        std::cout << d.getProperties().deviceName << "\n";
    }

    // Pick the best physical device
    auto physicalDevice = pickDevice(
        *instance,
        {
            .surface = *surface,
            .requiredExtensions = { { VK_KHR_SWAPCHAIN_EXTENSION_NAME } },
            .requiredQueueCapabilities = vk::QueueFlagBits::eGraphics,
        });

    // Get the queue family properties associated to this physical device
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // Get graphics queue family index
    const auto graphicsQueueIt = std::find_if(
        queueFamilyProperties.begin(),
        queueFamilyProperties.end(),
        [](vk::QueueFamilyProperties const& qfp) {
            return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
        });
    auto graphicsQueueFamilyIndex_ = std::distance(queueFamilyProperties.begin(), graphicsQueueIt);
    if (graphicsQueueFamilyIndex_ > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("graphicsQueueFamilyIndex above maximum value");
    }
    uint32_t graphicsQueueFamilyIndex = static_cast<uint32_t>(graphicsQueueFamilyIndex_);

    // Get present queue family index
    size_t presentQueueFamilyIndex_ = 0;
    for (auto i = 0ul; i < queueFamilyProperties.size(); i++) {
        if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface.get())) {
            presentQueueFamilyIndex_ = i;
            // TODO should break? if so, consider find_if
        }
    }
    if (presentQueueFamilyIndex_ > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("presentQueueFamilyIndex above maximum value");
    }
    uint32_t presentQueueFamilyIndex = static_cast<uint32_t>(presentQueueFamilyIndex_);

    std::cout << "graphicsQueueFamilyIndex: " << graphicsQueueFamilyIndex << "\n";
    std::cout << "presentQueueFamilyIndex: " << presentQueueFamilyIndex << "\n";

    // Create a list of queue family indices without duplicates
    std::set<uint32_t> uniqueQueueFamilyIndices_ {
        graphicsQueueFamilyIndex,
        presentQueueFamilyIndex,
    };
    std::vector<uint32_t> familyIndices(
        uniqueQueueFamilyIndices_.begin(),
        uniqueQueueFamilyIndices_.end());

    // Prepare the creation of each supported device queues
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 0;
    for (auto& queueFamilyIndex : familyIndices) {
        queueCreateInfos.push_back({
            .flags = {},
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        });
    }

    // Set wanted device extensions list
    const std::vector<const char*> deviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // Create a logical device associated to the physical device
    vk::UniqueDevice device = physicalDevice.createDeviceUnique({
        .flags = {},
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = 0u,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    });

    // Create queues for graphics and presentation
    vk::Queue deviceQueue = device->getQueue(graphicsQueueFamilyIndex, 0);
    vk::Queue presentQueue = device->getQueue(presentQueueFamilyIndex, 0);

    // Choose appropriate sharing mode based on queue family indexes
    struct SharingMode {
        vk::SharingMode sharingMode;
        uint32_t familyIndicesCount;
        uint32_t* familyIndicesDataPtr;
    };
    SharingMode sharingModeUtil = [&]() -> SharingMode {
        if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
            return { .sharingMode = vk::SharingMode::eConcurrent,
                     .familyIndicesCount = 2,
                     .familyIndicesDataPtr = familyIndices.data() };
        } else {
            return { .sharingMode = vk::SharingMode::eExclusive,
                     .familyIndicesCount = 0,
                     .familyIndicesDataPtr = nullptr };
        }
    }();

    // Get surface properties
    // Needed for validation warnings
    [[maybe_unused]] vk::SurfaceCapabilitiesKHR capabilities
        = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    [[maybe_unused]] std::vector<vk::SurfaceFormatKHR> formats
        = physicalDevice.getSurfaceFormatsKHR(*surface);

    // Create swapchain
    vk::UniqueSwapchainKHR swapChain = device->createSwapchainKHRUnique({
        .flags = {},
        .surface = surface.get(),
        .minImageCount = imageCount,
        .imageFormat = format,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = sharingModeUtil.sharingMode,
        .queueFamilyIndexCount = sharingModeUtil.familyIndicesCount,
        .pQueueFamilyIndices = sharingModeUtil.familyIndicesDataPtr,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = true,
        .oldSwapchain = nullptr,
    });

    // Create a list of images for the swapchain
    // The list size should match imageCount
    std::vector<vk::Image> swapChainImages = device->getSwapchainImagesKHR(swapChain.get());

    // Create an image view for each swapchain image
    std::vector<vk::UniqueImageView> swapChainImageViews;
    swapChainImageViews.reserve(swapChainImages.size());
    for (auto image : swapChainImages) {
        swapChainImageViews.push_back(device->createImageViewUnique({
            .flags = {},
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .components = {
                .r = vk::ComponentSwizzle::eR,
                .g = vk::ComponentSwizzle::eG,
                .b = vk::ComponentSwizzle::eB,
                .a = vk::ComponentSwizzle::eA,
            },
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        }));
    }

    // Create color image
    // TODO needed for multisampling
    // vk::UniqueImage colorImage = device->createImageUnique({
    //     .flags = {},
    //     .imageType = vk::ImageType::e2D,
    //     .format = format,
    //     .extent = { .width = extent.width, .height = extent.height, .depth = 1 },
    //     .mipLevels = 1,
    //     .arrayLayers = 1,
    //     .samples = msaaSamples,
    //     .tiling = vk::ImageTiling::eOptimal,
    //     .usage = vk::ImageUsageFlagBits::eColorAttachment
    //            | vk::ImageUsageFlagBits::eTransientAttachment,
    //     .sharingMode = vk::SharingMode::eExclusive,
    //     .queueFamilyIndexCount = 0,
    //     .pQueueFamilyIndices = nullptr,
    //     .initialLayout = vk::ImageLayout::eUndefined,
    // });
    // vk::UniqueImageView colorImageView = device->createImageViewUnique({
    //     .flags = {},
    //     .image = *colorImage,
    //     .viewType = vk::ImageViewType::e2D,
    //     .format = format,
    //     .components = {},
    //     .subresourceRange = {
    //         .aspectMask = vk::ImageAspectFlagBits::eColor,
    //         .baseMipLevel = 0,
    //         .levelCount = 1,
    //         .baseArrayLayer = 0,
    //         .layerCount = 1,
    //     },
    // });

    // Create depth stencil image
    // TODO needed for multisampling
    // vk::UniqueImage depthImage = device->createImageUnique({
    //     .flags = {},
    //     .imageType = vk::ImageType::e2D,
    //     .format = format,
    //     .extent = { .width = extent.width, .height = extent.height, .depth = 1 },
    //     .mipLevels = 1,
    //     .arrayLayers = 1,
    //     .samples = msaaSamples,
    //     .tiling = vk::ImageTiling::eOptimal,
    //     .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
    //     .sharingMode = vk::SharingMode::eExclusive,
    //     .queueFamilyIndexCount = 0,
    //     .pQueueFamilyIndices = nullptr,
    //     .initialLayout = vk::ImageLayout::eUndefined,
    // });
    // vk::UniqueImageView depthImageView = device->createImageViewUnique({
    //     .flags = {},
    //     .image = *depthImage,
    //     .viewType = vk::ImageViewType::e2D,
    //     .format = vk::Format::eD32Sfloat,
    //     .components = {},
    //     .subresourceRange = {
    //         .aspectMask = vk::ImageAspectFlagBits::eDepth,
    //         .baseMipLevel = 0,
    //         .levelCount = 1,
    //         .baseArrayLayer = 0,
    //         .layerCount = 1,
    //     },
    // });

    // Setup shaderc compiler
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    // Compile vertex shader
    shaderc::SpvCompilationResult vertShaderModule = compiler.CompileGlslToSpv(
        kVertexShaderSource,
        shaderc_glsl_vertex_shader,
        "vertex shader",
        options);
    if (vertShaderModule.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << vertShaderModule.GetErrorMessage();
    }
    std::vector<uint32_t> vertShaderCode(vertShaderModule.cbegin(), vertShaderModule.cend());
    auto vertSize = std::distance(vertShaderCode.begin(), vertShaderCode.end());
    vk::UniqueShaderModule vertexShaderModule = device->createShaderModuleUnique({
        .flags = {},
        .codeSize = vertSize * sizeof(uint32_t),
        .pCode = vertShaderCode.data(),
    });

    // Compile fragment shader
    shaderc::SpvCompilationResult fragShaderModule = compiler.CompileGlslToSpv(
        kFragmentShaderSource,
        shaderc_glsl_fragment_shader,
        "fragment shader",
        options);
    if (fragShaderModule.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << fragShaderModule.GetErrorMessage();
    }
    std::vector<uint32_t> fragShaderCode(fragShaderModule.cbegin(), fragShaderModule.cend());
    auto fragSize = std::distance(fragShaderCode.begin(), fragShaderCode.end());
    vk::UniqueShaderModule fragmentShaderModule = device->createShaderModuleUnique({
        .flags = {},
        .codeSize = fragSize * sizeof(uint32_t),
        .pCode = fragShaderCode.data(),
    });

    // Create render pass
    vk::AttachmentDescription colorAttachment {
        .flags = {},
        .format = format,
        .samples = msaaSamples,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    };
    vk::AttachmentReference colourAttachmentRef {
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal,
    };
    // TODO depth and resolve attachments needed for multisampling
    // vk::AttachmentDescription depthAttachment {
    //     .flags = {},
    //     .format = vk::Format::eD32Sfloat, // VK_FORMAT_D32_SFLOAT,
    //     VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT .samples =
    //     msaaSamples, .loadOp = vk::AttachmentLoadOp::eClear, .storeOp =
    //     vk::AttachmentStoreOp::eDontCare, .stencilLoadOp =
    //     vk::AttachmentLoadOp::eDontCare, .stencilStoreOp =
    //     vk::AttachmentStoreOp::eDontCare, .initialLayout =
    //     vk::ImageLayout::eUndefined, .finalLayout =
    //     vk::ImageLayout::eDepthStencilAttachmentOptimal,
    // };
    // vk::AttachmentReference depthAttachmentRef {
    //     .attachment = 1,
    //     .layout =  vk::ImageLayout::eDepthStencilAttachmentOptimal,
    // };
    // vk::AttachmentDescription resolveAttachment {
    //     .flags = {},
    //     .format = format,
    //     .samples = vk::SampleCountFlagBits::e1,
    //     .loadOp = vk::AttachmentLoadOp::eDontCare,
    //     .storeOp = vk::AttachmentStoreOp::eStore,
    //     .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
    //     .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
    //     .initialLayout = vk::ImageLayout::eUndefined,
    //     .finalLayout = vk::ImageLayout::ePresentSrcKHR,
    // };
    // vk::AttachmentReference resolveAttachmentRef {
    //     .attachment = 2,
    //     .layout =  vk::ImageLayout::eColorAttachmentOptimal,
    // };
    vk::SubpassDescription subpass {
        .flags = {},
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colourAttachmentRef,
        .pResolveAttachments = nullptr, // &resolveAttachmentRef,
        .pDepthStencilAttachment = nullptr, // &depthAttachmentRef,
        .preserveAttachmentCount = {},
        .pPreserveAttachments = {},
    };
    vk::SubpassDependency subpassDependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = {},
        .dstAccessMask
        = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        .dependencyFlags = {},
    };
    std::vector<vk::AttachmentDescription> attachmentDescriptions {
        colorAttachment,
        // depthAttachment,
        // resolveAttachment,
    };
    vk::UniqueRenderPass renderPass = device->createRenderPassUnique(vk::RenderPassCreateInfo {
        .flags = {},
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency,
    });

    // Create pipeline layout
    vk::UniquePipelineLayout pipelineLayout = device->createPipelineLayoutUnique({}, nullptr);

    // Create pipeline
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {
        .flags = {},
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *vertexShaderModule,
        .pName = "main",
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {
        .flags = {},
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *fragmentShaderModule,
        .pName = "main",
    };
    std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages {
        vertShaderStageInfo,
        fragShaderStageInfo,
    };
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
        .flags = {},
        .vertexBindingDescriptionCount
        = static_cast<uint32_t>(VertexInputDescription::bindings.size()),
        .pVertexBindingDescriptions = VertexInputDescription::bindings.data(),
        .vertexAttributeDescriptionCount
        = static_cast<uint32_t>(VertexInputDescription::attributes.size()),
        .pVertexAttributeDescriptions = VertexInputDescription::attributes.data(),
    };
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
        .flags = {},
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = false,
    };
    vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vk::Rect2D scissor {
        .offset = { .x = 0, .y = 0 },
        .extent = extent,
    };
    vk::PipelineViewportStateCreateInfo viewportState {
        .flags = {},
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };
    vk::PipelineRasterizationStateCreateInfo rasterizer {
        .flags = {},
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = {},
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = {},
        .depthBiasConstantFactor = {},
        .depthBiasClamp = {},
        .depthBiasSlopeFactor = {},
        .lineWidth = 1.0f,
    };
    vk::PipelineMultisampleStateCreateInfo multisampling {
        .flags = {},
        .rasterizationSamples = msaaSamples,
        .sampleShadingEnable = false,
        .minSampleShading = 1.0,
    };
    // TODO not used for now
    // vk::PipelineDepthStencilStateCreateInfo depthStencil {
    //     .flags = {},
    //     .depthTestEnable = true,
    //     .depthWriteEnable = true,
    //     .depthCompareOp = vk::CompareOp::eLess,
    //     .depthBoundsTestEnable = false,
    //     .stencilTestEnable = false,
    //     .front = {},
    //     .back = {},
    //     .minDepthBounds = 0.0f,
    //     .maxDepthBounds = 0.0f,
    // };
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = false,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eZero,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };
    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .flags = {},
        .logicOpEnable = false,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {},
    };
    auto [pipelineCreationResult, pipeline] = device->createGraphicsPipelineUnique(
        vk::PipelineCache {},
        {
            .flags = {},
            .stageCount = static_cast<uint32_t>(pipelineShaderStages.size()),
            .pStages = pipelineShaderStages.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pTessellationState = nullptr,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = nullptr, // &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = nullptr,
            .layout = *pipelineLayout,
            .renderPass = *renderPass,
            .subpass = 0,
            .basePipelineHandle = {},
            .basePipelineIndex = {},
        });
    if (pipelineCreationResult != vk::Result::eSuccess) {
        std::cerr << "Unable to create graphics pipeline: " << vk::to_string(pipelineCreationResult)
                  << "\n";
        return;
    }

    // Create semaphores for imageAvailable and renderFinished events
    vk::UniqueSemaphore imageAvailableSemaphore = device->createSemaphoreUnique({});
    vk::UniqueSemaphore renderFinishedSemaphore = device->createSemaphoreUnique({});

    // Create framebuffers
    std::vector<vk::UniqueFramebuffer> framebuffers(imageCount);
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array attachments {
            // *colorImageView,
            // *depthImageView,
            *swapChainImageViews[i],
        };
        framebuffers[i] = device->createFramebufferUnique({
            .flags = {},
            .renderPass = *renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
        });
    }

    // Create command pool
    vk::UniqueCommandPool commandPoolUnique = device->createCommandPoolUnique({
        .flags = {},
        .queueFamilyIndex = graphicsQueueFamilyIndex,
    });

    // Create command buffers
    std::vector<vk::UniqueCommandBuffer> commandBuffers = device->allocateCommandBuffersUnique({
        .commandPool = commandPoolUnique.get(),
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(framebuffers.size()),
    });

    // Create vertex buffer
    vk::UniqueBuffer vertexBuffer = device->createBufferUnique({
        .flags = {},
        .size = vertices.size() * sizeof(Vertex),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    });

    // Allocate device memory for that vertex buffer
    vk::MemoryRequirements memoryRequirements = device->getBufferMemoryRequirements(*vertexBuffer);
    uint32_t memoryTypeIndex = findMemoryType(
        physicalDevice.getMemoryProperties(),
        memoryRequirements.memoryTypeBits,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    vk::UniqueDeviceMemory deviceMemory = device->allocateMemoryUnique({
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    });

    // Copy the vertex and color data into that device memory
    auto* data = device->mapMemory(*deviceMemory, 0, memoryRequirements.size);
    std::memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));
    device->unmapMemory(*deviceMemory);

    // Bind the device memory to the vertex buffer
    device->bindBufferMemory(*vertexBuffer, *deviceMemory, 0);

    // Draw the vertices
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        commandBuffers[i]->begin(vk::CommandBufferBeginInfo {});
        vk::ClearValue clearValues {};
        vk::RenderPassBeginInfo renderPassBeginInfo {
            .renderPass = renderPass.get(),
            .framebuffer = framebuffers[i].get(),
            .renderArea = vk::Rect2D { { .x = 0, .y = 0 }, extent },
            .clearValueCount = 1,
            .pClearValues = &clearValues,
        };
        commandBuffers[i]->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        commandBuffers[i]->bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
        commandBuffers[i]->bindVertexBuffers(0, *vertexBuffer, { 0 });
        commandBuffers[i]->draw(vertices.size(), 1, 0, 0);
        commandBuffers[i]->endRenderPass();
        commandBuffers[i]->end();
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Get next image for rendering
        vk::ResultValue<uint32_t> imageIndex = device->acquireNextImageKHR(
            swapChain.get(),
            std::numeric_limits<uint64_t>::max(),
            imageAvailableSemaphore.get(),
            {});

        // Submit commands and acquired image to the graphics queue
        vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        vk::SubmitInfo submitInfo {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &imageAvailableSemaphore.get(),
            .pWaitDstStageMask = &waitStageMask,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffers[imageIndex.value].get(),
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &renderFinishedSemaphore.get(),
        };
        deviceQueue.submit(submitInfo, {});

        vk::Result result = presentQueue.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinishedSemaphore.get(),
            .swapchainCount = 1,
            .pSwapchains = &swapChain.get(),
            .pImageIndices = &imageIndex.value,
            .pResults = {},
        });
        if (result != vk::Result::eSuccess) {
            std::cerr << "Presentation error: " << vk::to_string(result) << "\n";
        }

        device->waitIdle();
    }
}

template<typename T, typename Deleter>
struct UniquePtr : public std::unique_ptr<T, Deleter> {
    using std::unique_ptr<T, Deleter>::unique_ptr;
    operator T*()
    {
        return this->get();
    }
};

template<auto deleter>
struct Deleter {
    template<typename... TArgs>
    decltype(auto) operator()(TArgs&&... args) const
    {
        return deleter(std::forward<TArgs>(args)...);
    }
};

template<typename T, auto allocator, auto deleter, typename... TArgs>
[[nodiscard]] UniquePtr<T, Deleter<deleter>> makeUnique(TArgs&&... args)
{
    return UniquePtr<T, Deleter<deleter>> { allocator(std::forward<TArgs>(args)...) };
}

/**
 * @brief Exception type for GLFW related errors
 */
class GlfwError : public std::runtime_error {
public:
    GlfwError(int error, const char* description)
        : std::runtime_error(std::string("from GLFW: ") + description)
        , error_ { error }
    {
    }

    [[nodiscard]] int error_code() const noexcept
    {
        return error_;
    }

private:
    const int error_;
};

/**
 * @brief RAII wrapper for GLFW setup and teardown operations
 */
class GlfwInstance {
public:
    /**
     * @brief Default constructor using the default error callback
     */
    GlfwInstance()
        : GlfwInstance(defaultErrorCallback)
    {
    }

    /**
     * @brief Delegated constructor performing GLFW initialization
     */
    GlfwInstance(GLFWerrorfun errorCallback)
    {
        glfwSetErrorCallback(errorCallback);
        [[maybe_unused]] auto initSuccessful = glfwInit();
        // If glfwInit fails, error_callback will be called.
        // So this return value can be ignored.
    }

    /**
     * @brief Destructor performing GLFW cleanup
     */
    ~GlfwInstance() noexcept
    {
        glfwTerminate();
    }

    // TODO disable copy/move

private:
    /**
     * @brief Default error callback throwing a GlfwError exception
     */
    static void defaultErrorCallback(int error, const char* description)
    {
        // TODO ensure throwing an exception from this callback is not undefined
        // behavior
        throw GlfwError(error, description);
    }
};

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

int main()
{
    GlfwInstance glfw;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    auto window = makeUnique<GLFWwindow, glfwCreateWindow, glfwDestroyWindow>(
        width,
        height,
        "Vulkan 101",
        nullptr,
        nullptr);
    if (window == nullptr) {
        throw std::runtime_error("Cannot create GLFW window");
    }

    glfwSetKeyCallback(window, key_callback);

    run(window);
}
