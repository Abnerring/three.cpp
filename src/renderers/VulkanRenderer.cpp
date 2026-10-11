#include "renderers/VulkanRenderer.h"
#include "cameras/Camera.h"
#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Frustum.h"
#include "math/Matrix4.h"
#include "objects/Mesh.h"
#include "scenes/FogExp2.h"
#include "scenes/Scene.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef near
#undef far
#else
#include <dlfcn.h>
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#ifndef THREECPP_SHADER_DIRECTORY
#define THREECPP_SHADER_DIRECTORY "shaders"
#endif

namespace {
void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + " failed (VkResult " +
                                 std::to_string(result) + ")");
}

#define INSTANCE_FUNCTIONS(X)                                                                      \
    X(vkDestroyInstance)                                                                           \
    X(vkEnumeratePhysicalDevices)                                                                  \
    X(vkGetPhysicalDeviceProperties) X(vkGetPhysicalDeviceMemoryProperties)                        \
        X(vkGetPhysicalDeviceQueueFamilyProperties) X(vkGetPhysicalDeviceFormatProperties)         \
            X(vkCreateDevice) X(vkGetDeviceProcAddr)
#define DEVICE_FUNCTIONS(X)                                                                        \
    X(vkDestroyDevice)                                                                             \
    X(vkGetDeviceQueue)                                                                            \
    X(vkDeviceWaitIdle) X(vkCreateImage) X(vkDestroyImage) X(vkGetImageMemoryRequirements) X(      \
        vkAllocateMemory) X(vkFreeMemory) X(vkBindImageMemory) X(vkCreateImageView)                \
        X(vkDestroyImageView) X(vkCreateBuffer) X(vkDestroyBuffer) X(                              \
            vkGetBufferMemoryRequirements) X(vkBindBufferMemory) X(vkMapMemory) X(vkUnmapMemory)   \
            X(vkCreateRenderPass) X(vkDestroyRenderPass) X(vkCreateFramebuffer) X(                 \
                vkDestroyFramebuffer) X(vkCreateShaderModule) X(vkDestroyShaderModule)             \
                X(vkCreatePipelineLayout) X(vkDestroyPipelineLayout) X(vkCreateGraphicsPipelines)  \
                    X(vkDestroyPipeline) X(vkCreateCommandPool) X(vkDestroyCommandPool) X(         \
                        vkAllocateCommandBuffers) X(vkResetCommandBuffer) X(vkBeginCommandBuffer)  \
                        X(vkEndCommandBuffer) X(vkCmdPipelineBarrier) X(vkCmdBeginRenderPass)      \
                            X(vkCmdEndRenderPass) X(vkCmdClearAttachments) X(vkCmdSetViewport)     \
                                X(vkCmdSetScissor) X(vkCmdBindPipeline) X(vkCmdBindVertexBuffers)  \
                                    X(vkCmdBindIndexBuffer) X(vkCmdPushConstants) X(vkCmdDraw)     \
                                        X(vkCmdDrawIndexed) X(vkCmdCopyImageToBuffer)              \
                                            X(vkCreateFence) X(vkDestroyFence) X(vkResetFences)    \
                                                X(vkQueueSubmit) X(vkWaitForFences)

struct VulkanApi {
#ifdef _WIN32
    HMODULE library = nullptr;
#else
    void* library = nullptr;
#endif
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    PFN_vkCreateInstance vkCreateInstance = nullptr;
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = nullptr;
    PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = nullptr;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
#define DECLARE_FUNCTION(name) PFN_##name name = nullptr;
    INSTANCE_FUNCTIONS(DECLARE_FUNCTION)
    DEVICE_FUNCTIONS(DECLARE_FUNCTION)
#undef DECLARE_FUNCTION

    VulkanApi() {
#ifdef _WIN32
        library = LoadLibraryA("vulkan-1.dll");
        if (library)
            vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                GetProcAddress(library, "vkGetInstanceProcAddr"));
#else
        library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
        if (library)
            vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
                dlsym(library, "vkGetInstanceProcAddr"));
#endif
        if (!vkGetInstanceProcAddr)
            throw std::runtime_error("Vulkan loader is unavailable");
        loadGlobal(vkCreateInstance, "vkCreateInstance");
        loadGlobal(vkEnumerateInstanceLayerProperties, "vkEnumerateInstanceLayerProperties");
        loadGlobal(vkEnumerateInstanceExtensionProperties,
                   "vkEnumerateInstanceExtensionProperties");
    }
    ~VulkanApi() {
#ifdef _WIN32
        if (library)
            FreeLibrary(library);
#else
        if (library)
            dlclose(library);
#endif
    }
    template <typename T> void loadGlobal(T& function, const char* name) {
        function = reinterpret_cast<T>(vkGetInstanceProcAddr(VK_NULL_HANDLE, name));
        if (!function)
            throw std::runtime_error(std::string("Vulkan entry point unavailable: ") + name);
    }
    void loadInstance(VkInstance instance) {
#define LOAD_INSTANCE(name)                                                                        \
    name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));                   \
    if (!name)                                                                                     \
        throw std::runtime_error("Missing " #name);
        INSTANCE_FUNCTIONS(LOAD_INSTANCE)
#undef LOAD_INSTANCE
        vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    }
    void loadDevice(VkDevice device) {
#define LOAD_DEVICE(name)                                                                          \
    name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name));                       \
    if (!name)                                                                                     \
        throw std::runtime_error("Missing " #name);
        DEVICE_FUNCTIONS(LOAD_DEVICE)
#undef LOAD_DEVICE
    }
};

struct Vertex {
    float position[3];
    float color[4];
    float viewDepth;
};
struct PushConstants {
    float mvp[16];
    float diffuse[4];
    float settings[4];
    float fogColor[4];
    float fogParams[4];
};
static_assert(sizeof(PushConstants) == 128);

std::vector<std::uint32_t> readShader(const char* filename) {
    const std::string path = std::string(THREECPP_SHADER_DIRECTORY) + "/" + filename;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Cannot read Vulkan shader: " + path);
    const auto length = file.tellg();
    if (length <= 0 || length % 4 != 0)
        throw std::runtime_error("Invalid SPIR-V shader: " + path);
    std::vector<std::uint32_t> words(static_cast<std::size_t>(length) / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(words.data()), length);
    if (!file || words.front() != 0x07230203)
        throw std::runtime_error("Invalid SPIR-V shader: " + path);
    return words;
}

float linearToSRGB(HIGH_PRECISION value) {
    return static_cast<float>(value <= 0.0031308 ? value * 12.92
                                                 : std::pow(value, 0.41666) * 1.055 - 0.055);
}

VkCompareOp depthCompare(int function, bool reversed) {
    static constexpr VkCompareOp operations[] = {
        VK_COMPARE_OP_NEVER,         VK_COMPARE_OP_ALWAYS,   VK_COMPARE_OP_LESS,
        VK_COMPARE_OP_LESS_OR_EQUAL, VK_COMPARE_OP_EQUAL,    VK_COMPARE_OP_GREATER_OR_EQUAL,
        VK_COMPARE_OP_GREATER,       VK_COMPARE_OP_NOT_EQUAL};
    if (function < 0 || function > 7)
        throw std::invalid_argument("Unsupported material.depthFunc");
    VkCompareOp result = operations[function];
    if (reversed) {
        if (result == VK_COMPARE_OP_LESS)
            result = VK_COMPARE_OP_GREATER;
        else if (result == VK_COMPARE_OP_LESS_OR_EQUAL)
            result = VK_COMPARE_OP_GREATER_OR_EQUAL;
        else if (result == VK_COMPARE_OP_GREATER)
            result = VK_COMPARE_OP_LESS;
        else if (result == VK_COMPARE_OP_GREATER_OR_EQUAL)
            result = VK_COMPARE_OP_LESS_OR_EQUAL;
    }
    return result;
}

std::array<HIGH_PRECISION, 4> transform(const Matrix4& matrix, HIGH_PRECISION x, HIGH_PRECISION y,
                                        HIGH_PRECISION z) {
    const auto e = matrix.elements();
    return {e[0] * x + e[4] * y + e[8] * z + e[12], e[1] * x + e[5] * y + e[9] * z + e[13],
            e[2] * x + e[6] * y + e[10] * z + e[14], e[3] * x + e[7] * y + e[11] * z + e[15]};
}

} // namespace

struct VulkanRenderer::Impl {
    VulkanApi api;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    std::uint32_t queueFamily = 0;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkShaderModule vertexShader = VK_NULL_HANDLE, fragmentShader = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    std::unordered_map<std::uint64_t, VkPipeline> pipelines;
    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    } colorImage, depthImage;
    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
    } readback;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    std::uint32_t width = 0, height = 0;
    bool initialized = false;
    bool recording = false;
    bool colorWrite = true;
    bool reversedDepth = false;
    Color clearColor{0, 0, 0};
    HIGH_PRECISION clearAlpha = 1;
    Color activeClearColor{0, 0, 0};
    HIGH_PRECISION activeClearAlpha = 1;
    bool activeClearSRGB = true;
    VulkanRendererInfo info;
    std::vector<std::uint8_t> pixels;
    mutable std::mutex validationMutex;
    std::vector<std::string> validationMessages;

    static VKAPI_ATTR VkBool32 VKAPI_CALL
    validationCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                       const VkDebugUtilsMessengerCallbackDataEXT* message, void* user) {
        auto* self = static_cast<Impl*>(user);
        std::lock_guard lock(self->validationMutex);
        self->validationMessages.emplace_back(message->pMessage ? message->pMessage
                                                                : "Vulkan validation message");
        return VK_FALSE;
    }

    Impl() = default;
    void initialize(const VulkanRendererParameters& parameters) {
        if (parameters.width == 0 || parameters.height == 0)
            throw std::invalid_argument("Renderer dimensions must be positive");
        VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        application.pApplicationName = "three.cpp";
        application.pEngineName = "three.cpp";
        application.apiVersion = VK_API_VERSION_1_1;
        std::vector<const char*> layers, extensions;
        VkDebugUtilsMessengerCreateInfoEXT debug{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug.pfnUserCallback = validationCallback;
        debug.pUserData = this;
        if (parameters.validation) {
            std::uint32_t count = 0;
            check(api.vkEnumerateInstanceLayerProperties(&count, nullptr),
                  "vkEnumerateInstanceLayerProperties");
            std::vector<VkLayerProperties> available(count);
            check(api.vkEnumerateInstanceLayerProperties(&count, available.data()),
                  "vkEnumerateInstanceLayerProperties");
            const bool found =
                std::any_of(available.begin(), available.end(), [](const auto& layer) {
                    return std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0;
                });
            if (!found)
                throw std::runtime_error("VK_LAYER_KHRONOS_validation is unavailable");
            layers.push_back("VK_LAYER_KHRONOS_validation");
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        create.pApplicationInfo = &application;
        create.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
        create.ppEnabledLayerNames = layers.data();
        create.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
        create.ppEnabledExtensionNames = extensions.data();
        if (parameters.validation)
            create.pNext = &debug;
        check(api.vkCreateInstance(&create, nullptr, &instance), "vkCreateInstance");
        api.loadInstance(instance);
        if (parameters.validation) {
            if (!api.vkCreateDebugUtilsMessengerEXT)
                throw std::runtime_error("Vulkan debug utils unavailable");
            check(api.vkCreateDebugUtilsMessengerEXT(instance, &debug, nullptr, &debugMessenger),
                  "vkCreateDebugUtilsMessengerEXT");
            info.validationEnabled = true;
        }
        std::uint32_t count = 0;
        check(api.vkEnumeratePhysicalDevices(instance, &count, nullptr),
              "vkEnumeratePhysicalDevices");
        std::vector<VkPhysicalDevice> devices(count);
        check(api.vkEnumeratePhysicalDevices(instance, &count, devices.data()),
              "vkEnumeratePhysicalDevices");
        int bestScore = -1;
        for (auto candidate : devices) {
            VkPhysicalDeviceProperties properties{};
            api.vkGetPhysicalDeviceProperties(candidate, &properties);
            if (properties.apiVersion < VK_API_VERSION_1_1)
                continue;
            std::uint32_t familyCount = 0;
            api.vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, nullptr);
            std::vector<VkQueueFamilyProperties> families(familyCount);
            api.vkGetPhysicalDeviceQueueFamilyProperties(candidate, &familyCount, families.data());
            for (std::uint32_t i = 0; i < familyCount; ++i) {
                if (!(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ||
                    families[i].queueCount == 0)
                    continue;
                const int score = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? 3
                                  : properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
                                      ? 2
                                      : 1;
                if (score > bestScore) {
                    bestScore = score;
                    physicalDevice = candidate;
                    queueFamily = i;
                    info.deviceName = properties.deviceName;
                }
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("No Vulkan 1.1 graphics device available");
        api.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
        float priority = 1;
        VkDeviceQueueCreateInfo queueCreate{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCreate.queueFamilyIndex = queueFamily;
        queueCreate.queueCount = 1;
        queueCreate.pQueuePriorities = &priority;
        VkDeviceCreateInfo deviceCreate{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceCreate.queueCreateInfoCount = 1;
        deviceCreate.pQueueCreateInfos = &queueCreate;
        check(api.vkCreateDevice(physicalDevice, &deviceCreate, nullptr, &device),
              "vkCreateDevice");
        api.loadDevice(device);
        api.vkGetDeviceQueue(device, queueFamily, 0, &queue);
        for (auto format : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM}) {
            VkFormatProperties properties{};
            api.vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);
            if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                depthFormat = format;
                break;
            }
        }
        if (depthFormat == VK_FORMAT_UNDEFINED)
            throw std::runtime_error("No supported Vulkan depth attachment format");
        VkCommandPoolCreateInfo poolCreate{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolCreate.queueFamilyIndex = queueFamily;
        poolCreate.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        check(api.vkCreateCommandPool(device, &poolCreate, nullptr, &commandPool),
              "vkCreateCommandPool");
        VkCommandBufferAllocateInfo commandAllocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandAllocate.commandPool = commandPool;
        commandAllocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandAllocate.commandBufferCount = 1;
        check(api.vkAllocateCommandBuffers(device, &commandAllocate, &commandBuffer),
              "vkAllocateCommandBuffers");
        VkFenceCreateInfo fenceCreate{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        check(api.vkCreateFence(device, &fenceCreate, nullptr, &fence), "vkCreateFence");
        createRenderPass();
        VkPushConstantRange range{};
        range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        range.size = sizeof(PushConstants);
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout.pushConstantRangeCount = 1;
        layout.pPushConstantRanges = &range;
        check(api.vkCreatePipelineLayout(device, &layout, nullptr, &pipelineLayout),
              "vkCreatePipelineLayout");
        vertexShader = createShader("basic.vert.spv");
        fragmentShader = createShader("basic.frag.spv");
        resize(parameters.width, parameters.height);
    }

    ~Impl() {
        if (device && api.vkDeviceWaitIdle)
            api.vkDeviceWaitIdle(device);
        if (device) {
            destroyTargets();
            for (const auto& entry : pipelines)
                api.vkDestroyPipeline(device, entry.second, nullptr);
            if (pipelineLayout)
                api.vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            if (vertexShader)
                api.vkDestroyShaderModule(device, vertexShader, nullptr);
            if (fragmentShader)
                api.vkDestroyShaderModule(device, fragmentShader, nullptr);
            if (renderPass)
                api.vkDestroyRenderPass(device, renderPass, nullptr);
            if (fence)
                api.vkDestroyFence(device, fence, nullptr);
            if (commandPool)
                api.vkDestroyCommandPool(device, commandPool, nullptr);
            if (api.vkDestroyDevice)
                api.vkDestroyDevice(device, nullptr);
        }
        if (debugMessenger && api.vkDestroyDebugUtilsMessengerEXT)
            api.vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        if (instance && api.vkDestroyInstance)
            api.vkDestroyInstance(instance, nullptr);
    }

    std::uint32_t memoryType(std::uint32_t bits, VkMemoryPropertyFlags flags) {
        for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
            if ((bits & (1u << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
                return i;
        throw std::runtime_error("Required Vulkan memory type unavailable");
    }
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data = nullptr) {
        Buffer result;
        result.size = size;
        try {
            VkBufferCreateInfo create{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            create.size = size;
            create.usage = usage;
            create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            check(api.vkCreateBuffer(device, &create, nullptr, &result.buffer), "vkCreateBuffer");
            VkMemoryRequirements requirements{};
            api.vkGetBufferMemoryRequirements(device, result.buffer, &requirements);
            VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocate.allocationSize = requirements.size;
            allocate.memoryTypeIndex =
                memoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            check(api.vkAllocateMemory(device, &allocate, nullptr, &result.memory),
                  "vkAllocateMemory");
            check(api.vkBindBufferMemory(device, result.buffer, result.memory, 0),
                  "vkBindBufferMemory");
            if (data) {
                void* mapped;
                check(api.vkMapMemory(device, result.memory, 0, size, 0, &mapped), "vkMapMemory");
                std::memcpy(mapped, data, static_cast<std::size_t>(size));
                api.vkUnmapMemory(device, result.memory);
            }
        } catch (...) {
            destroyBuffer(result);
            throw;
        }
        return result;
    }
    void destroyBuffer(Buffer& buffer) {
        if (buffer.buffer)
            api.vkDestroyBuffer(device, buffer.buffer, nullptr);
        if (buffer.memory)
            api.vkFreeMemory(device, buffer.memory, nullptr);
        buffer = {};
    }
    void createImage(Image& result, VkFormat format, VkImageUsageFlags usage,
                     VkImageAspectFlags aspect) {
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        create.imageType = VK_IMAGE_TYPE_2D;
        create.format = format;
        create.extent = {width, height, 1};
        create.mipLevels = 1;
        create.arrayLayers = 1;
        create.samples = VK_SAMPLE_COUNT_1_BIT;
        create.tiling = VK_IMAGE_TILING_OPTIMAL;
        create.usage = usage;
        create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        check(api.vkCreateImage(device, &create, nullptr, &result.image), "vkCreateImage");
        VkMemoryRequirements requirements{};
        api.vkGetImageMemoryRequirements(device, result.image, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex =
            memoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(api.vkAllocateMemory(device, &allocate, nullptr, &result.memory), "vkAllocateMemory");
        check(api.vkBindImageMemory(device, result.image, result.memory, 0), "vkBindImageMemory");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = result.image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange.aspectMask = aspect;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;
        check(api.vkCreateImageView(device, &view, nullptr, &result.view), "vkCreateImageView");
    }
    void destroyImage(Image& image) {
        if (image.view)
            api.vkDestroyImageView(device, image.view, nullptr);
        if (image.image)
            api.vkDestroyImage(device, image.image, nullptr);
        if (image.memory)
            api.vkFreeMemory(device, image.memory, nullptr);
        image = {};
    }
    void destroyTargets() {
        if (framebuffer)
            api.vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
        destroyImage(colorImage);
        destroyImage(depthImage);
        destroyBuffer(readback);
    }
    void resize(std::uint32_t newWidth, std::uint32_t newHeight) {
        if (!newWidth || !newHeight)
            throw std::invalid_argument("Renderer dimensions must be positive");
        VkPhysicalDeviceProperties properties{};
        api.vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        if (newWidth > properties.limits.maxFramebufferWidth ||
            newHeight > properties.limits.maxFramebufferHeight)
            throw std::invalid_argument("Renderer dimensions exceed Vulkan device limits");
        if (newWidth == width && newHeight == height)
            return;
        check(api.vkDeviceWaitIdle(device), "vkDeviceWaitIdle");
        destroyTargets();
        width = newWidth;
        height = newHeight;
        initialized = false;
        createImage(colorImage, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT);
        createImage(depthImage, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT);
        const VkImageView views[] = {colorImage.view, depthImage.view};
        VkFramebufferCreateInfo create{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        create.renderPass = renderPass;
        create.attachmentCount = 2;
        create.pAttachments = views;
        create.width = width;
        create.height = height;
        create.layers = 1;
        check(api.vkCreateFramebuffer(device, &create, nullptr, &framebuffer),
              "vkCreateFramebuffer");
        readback = createBuffer(static_cast<VkDeviceSize>(width) * height * 4,
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        pixels.assign(static_cast<std::size_t>(width) * height * 4, 0);
    }
    void createRenderPass() {
        VkAttachmentDescription attachments[2]{};
        attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = attachments[0].finalLayout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[1] = attachments[0];
        attachments[1].format = depthFormat;
        attachments[1].initialLayout = attachments[1].finalLayout =
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colorReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
            depthReference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;
        subpass.pDepthStencilAttachment = &depthReference;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo create{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        create.attachmentCount = 2;
        create.pAttachments = attachments;
        create.subpassCount = 1;
        create.pSubpasses = &subpass;
        create.dependencyCount = 1;
        create.pDependencies = &dependency;
        check(api.vkCreateRenderPass(device, &create, nullptr, &renderPass), "vkCreateRenderPass");
    }
    VkShaderModule createShader(const char* filename) {
        const auto words = readShader(filename);
        VkShaderModuleCreateInfo create{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        create.codeSize = words.size() * 4;
        create.pCode = words.data();
        VkShaderModule shader;
        check(api.vkCreateShaderModule(device, &create, nullptr, &shader), "vkCreateShaderModule");
        return shader;
    }

    VkPipeline pipeline(const MeshBasicMaterial& material, int side, bool mirrored, bool reverse) {
        if (side < 0 || side > 2 || material.blending < 0 || material.blending > 4 ||
            material.depthFunc < 0 || material.depthFunc > 7)
            throw std::invalid_argument(
                "Unsupported material side, blending mode or depth function");
        const std::uint64_t key =
            static_cast<std::uint64_t>(side) | (std::uint64_t(mirrored) << 2) |
            (std::uint64_t(material.depthTest) << 3) | (std::uint64_t(material.depthWrite) << 4) |
            (std::uint64_t(material.colorWrite) << 5) | (std::uint64_t(material.transparent) << 6) |
            (std::uint64_t(material.blending) << 7) |
            (std::uint64_t(material.premultipliedAlpha) << 10) |
            (std::uint64_t(material.depthFunc) << 11) | (std::uint64_t(material.wireframe) << 15) |
            (std::uint64_t(reverse) << 16);
        if (auto found = pipelines.find(key); found != pipelines.end())
            return found->second;
        VkPipelineShaderStageCreateInfo stages[2]{};
        for (auto& stage : stages) {
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.pName = "main";
        }
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertexShader;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragmentShader;
        VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        const VkVertexInputAttributeDescription attributes[] = {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
            {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 12},
            {2, 0, VK_FORMAT_R32_SFLOAT, 28}};
        VkPipelineVertexInputStateCreateInfo vertex{
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex.vertexBindingDescriptionCount = 1;
        vertex.pVertexBindingDescriptions = &binding;
        vertex.vertexAttributeDescriptionCount = 3;
        vertex.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = material.wireframe ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST
                                               : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rasterization{
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization.cullMode = side == 2   ? VK_CULL_MODE_NONE
                                 : side == 1 ? VK_CULL_MODE_FRONT_BIT
                                             : VK_CULL_MODE_BACK_BIT;
        // Negating clip-space Y converts three.js' upward axis to top-down image rows.
        rasterization.frontFace =
            mirrored ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo multisample{
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = material.depthTest;
        depth.depthWriteEnable = material.depthWrite;
        depth.depthCompareOp = depthCompare(material.depthFunc, reverse);
        VkPipelineColorBlendAttachmentState blend{};
        blend.colorWriteMask = material.colorWrite
                                   ? VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
                                   : 0;
        blend.blendEnable =
            material.blending != 0 && !(material.blending == 1 && !material.transparent);
        blend.colorBlendOp = blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.srcColorBlendFactor =
            material.premultipliedAlpha ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        if (material.blending == 2) {
            blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        } else if (material.blending == 3) {
            blend.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blend.dstAlphaBlendFactor = material.premultipliedAlpha
                                            ? VK_BLEND_FACTOR_ONE
                                            : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        } else if (material.blending == 4) {
            blend.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            blend.dstColorBlendFactor = material.premultipliedAlpha
                                            ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
                                            : VK_BLEND_FACTOR_ZERO;
            blend.srcAlphaBlendFactor =
                material.premultipliedAlpha ? VK_BLEND_FACTOR_ZERO : VK_BLEND_FACTOR_DST_ALPHA;
            blend.dstAlphaBlendFactor =
                material.premultipliedAlpha ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ZERO;
        }
        VkPipelineColorBlendStateCreateInfo colorBlend{
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blend;
        const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                                VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamicStates;
        VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        create.stageCount = 2;
        create.pStages = stages;
        create.pVertexInputState = &vertex;
        create.pInputAssemblyState = &assembly;
        create.pViewportState = &viewport;
        create.pRasterizationState = &rasterization;
        create.pMultisampleState = &multisample;
        create.pDepthStencilState = &depth;
        create.pColorBlendState = &colorBlend;
        create.pDynamicState = &dynamic;
        create.layout = pipelineLayout;
        create.renderPass = renderPass;
        VkPipeline result;
        check(api.vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create, nullptr, &result),
              "vkCreateGraphicsPipelines");
        pipelines.emplace(key, result);
        return result;
    }

    void imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout,
                      VkImageLayout newLayout, VkAccessFlags sourceAccess,
                      VkAccessFlags destinationAccess, VkPipelineStageFlags sourceStage,
                      VkPipelineStageFlags destinationStage) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask = sourceAccess;
        barrier.dstAccessMask = destinationAccess;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspect;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        api.vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0,
                                 nullptr, 1, &barrier);
    }
    void beginFrame(bool clearColorAttachment, bool clearDepthAttachment, const Color& color,
                    HIGH_PRECISION alpha, bool srgb) {
        check(api.vkResetCommandBuffer(commandBuffer, 0), "vkResetCommandBuffer");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(api.vkBeginCommandBuffer(commandBuffer, &begin), "vkBeginCommandBuffer");
        if (!initialized) {
            imageBarrier(colorImage.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
                         VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
            imageBarrier(depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0,
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        }
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        render.renderPass = renderPass;
        render.framebuffer = framebuffer;
        render.renderArea.extent = {width, height};
        api.vkCmdBeginRenderPass(commandBuffer, &render, VK_SUBPASS_CONTENTS_INLINE);
        recording = true;
        clearAttachments(clearColorAttachment || !initialized, clearDepthAttachment || !initialized,
                         color, alpha, srgb);
        VkViewport viewport{0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1};
        VkRect2D scissor{{0, 0}, {width, height}};
        api.vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        api.vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }
    void clearAttachments(bool clearColorAttachment, bool clearDepthAttachment, const Color& color,
                          HIGH_PRECISION alpha, bool srgb) {
        std::vector<VkClearAttachment> attachments;
        if (clearColorAttachment) {
            VkClearAttachment attachment{};
            attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            attachment.clearValue.color.float32[0] =
                srgb ? linearToSRGB(color.r()) : static_cast<float>(color.r());
            attachment.clearValue.color.float32[1] =
                srgb ? linearToSRGB(color.g()) : static_cast<float>(color.g());
            attachment.clearValue.color.float32[2] =
                srgb ? linearToSRGB(color.b()) : static_cast<float>(color.b());
            attachment.clearValue.color.float32[3] = static_cast<float>(alpha);
            attachments.push_back(attachment);
        }
        if (clearDepthAttachment) {
            VkClearAttachment attachment{};
            attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            attachment.clearValue.depthStencil.depth = reversedDepth ? 0 : 1;
            attachments.push_back(attachment);
        }
        VkClearRect rect{};
        rect.rect.extent = {width, height};
        rect.layerCount = 1;
        if (!attachments.empty())
            api.vkCmdClearAttachments(commandBuffer, static_cast<std::uint32_t>(attachments.size()),
                                      attachments.data(), 1, &rect);
    }
    void abortFrame() {
        if (recording) {
            api.vkResetCommandBuffer(commandBuffer, 0);
            recording = false;
        }
    }
    void endFrame() {
        api.vkCmdEndRenderPass(commandBuffer);
        imageBarrier(colorImage.image, VK_IMAGE_ASPECT_COLOR_BIT,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = {width, height, 1};
        api.vkCmdCopyImageToBuffer(commandBuffer, colorImage.image,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &copy);
        imageBarrier(colorImage.image, VK_IMAGE_ASPECT_COLOR_BIT,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_TRANSFER_READ_BIT,
                     VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        VkMemoryBarrier host{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        api.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host, 0, nullptr, 0, nullptr);
        check(api.vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");
        recording = false;
        check(api.vkResetFences(device, 1, &fence), "vkResetFences");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commandBuffer;
        check(api.vkQueueSubmit(queue, 1, &submit, fence), "vkQueueSubmit");
        check(api.vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
        void* mapped;
        check(api.vkMapMemory(device, readback.memory, 0, readback.size, 0, &mapped),
              "vkMapMemory");
        std::memcpy(pixels.data(), mapped, pixels.size());
        api.vkUnmapMemory(device, readback.memory);
        initialized = true;
    }
};

VulkanRenderer::VulkanRenderer(const VulkanRendererParameters& parameters)
    : impl(std::make_unique<Impl>()) {
    impl->initialize(parameters);
}
VulkanRenderer::~VulkanRenderer() = default;
void VulkanRenderer::setSize(std::uint32_t width, std::uint32_t height) {
    if (impl->recording)
        throw std::logic_error("setSize inside an active render callback is not supported yet");
    impl->resize(width, height);
}
std::uint32_t VulkanRenderer::getWidth() const {
    return impl->width;
}
std::uint32_t VulkanRenderer::getHeight() const {
    return impl->height;
}
void VulkanRenderer::setClearColor(const Color& color, HIGH_PRECISION alpha) {
    impl->clearColor.copy(color);
    impl->clearAlpha = alpha;
    impl->activeClearColor.copy(color);
    impl->activeClearAlpha = alpha;
    impl->activeClearSRGB = srgbOutput;
}
const Color& VulkanRenderer::getClearColor() const {
    return impl->clearColor;
}
HIGH_PRECISION VulkanRenderer::getClearAlpha() const {
    return impl->clearAlpha;
}
void VulkanRenderer::clear(bool color, bool depth) {
    if (impl->recording) {
        impl->clearAttachments(color && impl->colorWrite, depth, impl->activeClearColor,
                               impl->activeClearAlpha, impl->activeClearSRGB);
        return;
    }
    impl->beginFrame(color && impl->colorWrite, depth, impl->activeClearColor,
                     impl->activeClearAlpha, impl->activeClearSRGB);
    impl->endFrame();
}
std::vector<std::uint8_t> VulkanRenderer::readPixels() const {
    if (impl->recording) {
        impl->endFrame();
        // Preserve attachments while subsequent callbacks and draws continue this frame.
        impl->beginFrame(false, false, impl->clearColor, impl->clearAlpha, srgbOutput);
    }
    return impl->pixels;
}
const VulkanRendererInfo& VulkanRenderer::getInfo() const {
    return impl->info;
}
std::vector<std::string> VulkanRenderer::getValidationMessages() const {
    std::lock_guard lock(impl->validationMutex);
    return impl->validationMessages;
}

void VulkanRenderer::render(Scene& scene, Camera& camera) {
    if (impl->recording)
        throw std::logic_error(
            "Nested render inside an active render callback is not supported yet");
    // Hooks may detach the camera or release the caller's last managed scene reference.
    [[maybe_unused]] const auto sceneLifetime = scene.weak_from_this().lock();
    [[maybe_unused]] const auto cameraLifetime = camera.weak_from_this().lock();
    if (camera.coordinateSystem != 2000 && camera.coordinateSystem != 2001)
        throw std::invalid_argument("Unknown camera coordinate system");
    if (scene.matrixWorldAutoUpdate)
        scene.updateMatrixWorld();
    if (camera.parent == nullptr && camera.matrixWorldAutoUpdate)
        camera.updateMatrixWorld();
    if (scene.onBeforeRender)
        scene.onBeforeRender(*this, scene, camera, nullptr, nullptr, nullptr);
    impl->reversedDepth = camera.reversedDepth;
    Matrix4 viewProjection;
    viewProjection.multiplyMatrices(camera.projectionMatrix, camera.matrixWorldInverse);
    Frustum frustum;
    frustum.setFromProjectionMatrix(viewProjection, camera.coordinateSystem, camera.reversedDepth);
    struct Item {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<BufferGeometry> geometry;
        std::shared_ptr<Material> material;
        std::shared_ptr<BufferGeometry::Group> group;
        int groupOrder;
        HIGH_PRECISION z;
    };
    std::vector<Item> opaque, transparent;
    auto project = [&](auto&& self, Object3D& object, int groupOrder) -> void {
        if (!object.visible)
            return;
        if (object.layers.test(camera.layers)) {
            if (object.isGroup)
                groupOrder = object.renderOrder;
            if (auto* mesh = dynamic_cast<Mesh*>(&object); mesh && mesh->geometry) {
                const auto position = mesh->geometry->getAttribute("position");
                if (position && position->count) {
                    if (position->itemSize < 3)
                        throw std::invalid_argument(
                            "Mesh position attribute requires at least three components");
                    if ((object.frustumCulled || sortObjects) && !mesh->geometry->boundingSphere)
                        mesh->geometry->computeBoundingSphere();
                    Sphere sphere(Vector3(), 0);
                    if (mesh->geometry->boundingSphere) {
                        sphere.copy(*mesh->geometry->boundingSphere);
                    }
                    sphere.applyMatrix4(object.matrixWorld);
                    if (!object.frustumCulled || frustum.intersectsSphere(sphere)) {
                        Vector3 projected(sphere.center);
                        projected.applyMatrix4(viewProjection);
                        const auto z = projected.z() * (camera.reversedDepth ? -1 : 1);
                        auto add = [&](const std::shared_ptr<Material>& material,
                                       std::shared_ptr<BufferGeometry::Group> group) {
                            if (!material || !material->visible)
                                return;
                            auto meshLifetime =
                                std::dynamic_pointer_cast<Mesh>(mesh->weak_from_this().lock());
                            if (!meshLifetime)
                                throw std::invalid_argument(
                                    "Scene meshes must use shared_ptr ownership");
                            Item item{std::move(meshLifetime),
                                      mesh->geometry,
                                      material,
                                      group,
                                      groupOrder,
                                      z};
                            (material->transparent ? transparent : opaque)
                                .push_back(std::move(item));
                        };
                        if (mesh->materialIsArray || !mesh->materials.empty()) {
                            for (const auto& group : mesh->geometry->groups) {
                                if (group.materialIndex >= mesh->materials.size())
                                    continue;
                                add(mesh->materials[group.materialIndex],
                                    mesh->geometry->groups.owner(group));
                            }
                        } else
                            add(mesh->material, nullptr);
                    }
                }
            }
        }
        for (const auto& child : object.children)
            if (child)
                self(self, *child, groupOrder);
    };
    project(project, scene, 0);
    if (sortObjects) {
        std::stable_sort(opaque.begin(), opaque.end(), [](const Item& a, const Item& b) {
            if (a.groupOrder != b.groupOrder)
                return a.groupOrder < b.groupOrder;
            if (a.mesh->renderOrder != b.mesh->renderOrder)
                return a.mesh->renderOrder < b.mesh->renderOrder;
            if (a.material->id != b.material->id)
                return a.material->id < b.material->id;
            if (a.z != b.z)
                return a.z < b.z;
            return a.mesh->id < b.mesh->id;
        });
        std::stable_sort(transparent.begin(), transparent.end(), [](const Item& a, const Item& b) {
            if (a.groupOrder != b.groupOrder)
                return a.groupOrder < b.groupOrder;
            if (a.mesh->renderOrder != b.mesh->renderOrder)
                return a.mesh->renderOrder < b.mesh->renderOrder;
            if (a.z != b.z)
                return a.z > b.z;
            return a.mesh->id < b.mesh->id;
        });
    }
    // Retain uploaded buffers until every recorded draw has completed on the GPU.
    struct Prepared {
        Impl::Buffer vertices, indices;
        VkPipeline pipeline;
        PushConstants constants;
        std::uint32_t start, count;
        bool indexed, wireframe, colorWrite;
    };
    std::vector<Prepared> prepared;
    auto cleanup = [&]() {
        for (auto& draw : prepared) {
            impl->destroyBuffer(draw.vertices);
            impl->destroyBuffer(draw.indices);
        }
    };
    try {
        impl->info.calls = impl->info.triangles = impl->info.lines = 0;
        ++impl->info.frame;
        const bool forceClear = autoClear || scene.background.has_value();
        impl->activeClearColor.copy(scene.background ? *scene.background : impl->clearColor);
        impl->activeClearAlpha = scene.background ? 1 : impl->clearAlpha;
        impl->activeClearSRGB = srgbOutput;
        if (forceClear)
            impl->colorWrite = true;
        impl->beginFrame(forceClear && autoClearColor, forceClear && autoClearDepth,
                         impl->activeClearColor, impl->activeClearAlpha, impl->activeClearSRGB);
        auto prepare = [&](const Item& item, const std::shared_ptr<Material>& effectiveMaterial,
                           int side) {
            const auto& mesh = *item.mesh;
            auto basic = std::dynamic_pointer_cast<MeshBasicMaterial>(effectiveMaterial);
            if (!basic)
                throw std::invalid_argument(
                    "VulkanRenderer currently supports MeshBasicMaterial only");
            const auto& material = *basic;
            if (material.map || material.lightMap || material.aoMap || material.specularMap ||
                material.alphaMap || material.envMap)
                throw std::invalid_argument("Vulkan texture sampling is not implemented yet");
            if (material.alphaHash || material.alphaToCoverage || material.stencilWrite ||
                material.dithering ||
                (material.clippingPlanes && !material.clippingPlanes->empty()))
                throw std::invalid_argument("Vulkan alphaHash, alphaToCoverage, stencil, dithering "
                                            "and clipping are not implemented yet");
            if (material.polygonOffset || (material.wireframe && material.wireframeLinewidth != 1))
                throw std::invalid_argument(
                    "Vulkan polygon offset and wide wireframe lines are not implemented yet");
            if ((material.blending == 3 || material.blending == 4) && !material.premultipliedAlpha)
                throw std::invalid_argument(
                    "three.js requires premultipliedAlpha for subtractive and multiply blending");
            if (item.geometry->morphAttributes.contains("color"))
                throw std::invalid_argument("Vulkan morph colors are not implemented yet");
            const Matrix4& modelView = mesh.modelViewMatrix;
            const auto position = item.geometry->getAttribute("position");
            if (!position || position->count == 0)
                return;
            const auto geometryCount =
                item.geometry->index ? item.geometry->index->count : position->count;
            auto start = std::min(item.geometry->drawRange.start, geometryCount);
            auto end = start + std::min(item.geometry->drawRange.count, geometryCount - start);
            if (item.group) {
                const auto& group = *item.group;
                start = std::max(start, group.start);
                const auto groupEnd =
                    group.start > geometryCount
                        ? geometryCount
                        : group.start + std::min(group.count, geometryCount - group.start);
                end = std::min(end, groupEnd);
            }
            if (end <= start)
                return;
            const auto count = end - start;
            auto colors = material.vertexColors ? item.geometry->getAttribute("color") : nullptr;
            if (colors && (colors->count < position->count || colors->itemSize < 3))
                throw std::invalid_argument("Invalid mesh color attribute");
            std::vector<Vertex> vertices(position->count);
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                Vector3 deformed(position->getX(i), position->getY(i), position->getZ(i));
                if (item.geometry == mesh.geometry)
                    mesh.getVertexPosition(i, deformed);
                else if (item.geometry->morphAttributes.contains("position"))
                    throw std::invalid_argument(
                        "Changing mesh.geometry during morph rendering is not supported yet");
                if (!std::isfinite(deformed.x()) || !std::isfinite(deformed.y()) ||
                    !std::isfinite(deformed.z()))
                    throw std::invalid_argument("Non-finite geometry position");
                const auto viewPosition =
                    transform(modelView, deformed.x(), deformed.y(), deformed.z());
                vertices[i] = {{static_cast<float>(deformed.x()), static_cast<float>(deformed.y()),
                                static_cast<float>(deformed.z())},
                               {1, 1, 1, 1},
                               static_cast<float>(-viewPosition[2])};
                if (colors) {
                    vertices[i].color[0] = static_cast<float>(colors->getX(i));
                    vertices[i].color[1] = static_cast<float>(colors->getY(i));
                    vertices[i].color[2] = static_cast<float>(colors->getZ(i));
                    if (colors->itemSize == 4)
                        vertices[i].color[3] = static_cast<float>(colors->getW(i));
                }
            }
            std::vector<std::uint32_t> indices;
            auto index = item.geometry->index;
            if (index) {
                if (index->itemSize != 1)
                    throw std::invalid_argument("Mesh index attribute requires one component");
                indices.resize(index->count);
                // Index fetches use the array's raw integers; normalized only affects vertex
                // attributes.
                std::visit(
                    [&](const auto& values) {
                        using Component = typename std::decay_t<decltype(values)>::value_type;
                        if constexpr (std::is_same_v<Component, std::uint8_t> ||
                                      std::is_same_v<Component, std::uint16_t> ||
                                      std::is_same_v<Component, std::uint32_t>) {
                            if (values.size() < indices.size())
                                throw std::invalid_argument("Mesh index count exceeds its array");
                            for (std::size_t i = 0; i < indices.size(); ++i) {
                                if (values[i] >= vertices.size())
                                    throw std::invalid_argument(
                                        "Mesh index is outside the position attribute");
                                indices[i] = static_cast<std::uint32_t>(values[i]);
                            }
                        } else
                            throw std::invalid_argument(
                                "Mesh index array must use unsigned 8, 16 or 32-bit integers");
                    },
                    index->array);
            }
            if (material.wireframe) {
                std::vector<std::uint32_t> edges;
                const auto count = index ? indices.size() : vertices.size();
                edges.reserve(count * 2);
                for (std::size_t i = 0; i + 2 < count; i += 3) {
                    const auto a = index ? indices[i] : static_cast<std::uint32_t>(i),
                               b = index ? indices[i + 1] : static_cast<std::uint32_t>(i + 1),
                               c = index ? indices[i + 2] : static_cast<std::uint32_t>(i + 2);
                    edges.insert(edges.end(), {a, b, b, c, c, a});
                }
                indices = std::move(edges);
            }
            const auto factor = material.wireframe ? 2u : 1u;
            if (vertices.size() > UINT32_MAX || start > UINT32_MAX / factor ||
                count > UINT32_MAX / factor)
                throw std::invalid_argument("Geometry exceeds Vulkan draw limits");
            Prepared draw{};
            draw.start = static_cast<std::uint32_t>(start * factor);
            draw.count = static_cast<std::uint32_t>(count * factor);
            draw.indexed = index != nullptr || material.wireframe;
            draw.wireframe = material.wireframe;
            draw.colorWrite = material.colorWrite;
            if (draw.indexed) {
                if (draw.start >= indices.size())
                    return;
                draw.count =
                    std::min(draw.count, static_cast<std::uint32_t>(indices.size()) - draw.start);
            }
            Matrix4 mvp;
            mvp.multiplyMatrices(camera.projectionMatrix, modelView);
            Matrix4 correction;
            // three.js uses a zero-to-one clip range for reversed depth on either backend.
            const bool openGL = camera.coordinateSystem == 2000 && !camera.reversedDepth;
            correction.set(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, openGL ? 0.5 : 1, openGL ? 0.5 : 0, 0, 0,
                           0, 1);
            mvp.premultiply(correction);
            const auto elements = mvp.elements();
            for (int i = 0; i < 16; ++i)
                draw.constants.mvp[i] = static_cast<float>(elements[i]);
            draw.constants.diffuse[0] = static_cast<float>(material.color.r());
            draw.constants.diffuse[1] = static_cast<float>(material.color.g());
            draw.constants.diffuse[2] = static_cast<float>(material.color.b());
            draw.constants.diffuse[3] = static_cast<float>(material.opacity);
            draw.constants.settings[0] = static_cast<float>(material.alphaTest);
            draw.constants.settings[1] =
                !material.transparent && material.blending == 1 && !material.alphaToCoverage ? 1.0f
                                                                                             : 0.0f;
            draw.constants.settings[2] = material.premultipliedAlpha ? 1.0f : 0.0f;
            draw.constants.settings[3] = srgbOutput ? 1.0f : 0.0f;
            if (scene.fog && material.fog) {
                const Color* fogColor;
                if (auto fog = std::dynamic_pointer_cast<Fog>(scene.fog)) {
                    fogColor = &fog->color;
                    draw.constants.fogParams[0] = 1;
                    draw.constants.fogParams[1] = static_cast<float>(fog->near);
                    draw.constants.fogParams[2] = static_cast<float>(fog->far);
                } else if (auto fog = std::dynamic_pointer_cast<FogExp2>(scene.fog)) {
                    fogColor = &fog->color;
                    draw.constants.fogParams[0] = 2;
                    draw.constants.fogParams[1] = static_cast<float>(fog->density);
                } else
                    throw std::invalid_argument("Unknown scene fog type");
                draw.constants.fogColor[0] =
                    srgbOutput ? linearToSRGB(fogColor->r()) : static_cast<float>(fogColor->r());
                draw.constants.fogColor[1] =
                    srgbOutput ? linearToSRGB(fogColor->g()) : static_cast<float>(fogColor->g());
                draw.constants.fogColor[2] =
                    srgbOutput ? linearToSRGB(fogColor->b()) : static_cast<float>(fogColor->b());
            }
            Matrix4 world;
            world.copy(mesh.matrixWorld);
            draw.pipeline =
                impl->pipeline(material, side, world.determinant() < 0, camera.reversedDepth);
            try {
                draw.vertices =
                    impl->createBuffer(vertices.size() * sizeof(Vertex),
                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.data());
                if (draw.indexed)
                    draw.indices =
                        impl->createBuffer(indices.size() * sizeof(std::uint32_t),
                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices.data());
                prepared.push_back(draw);
            } catch (...) {
                impl->destroyBuffer(draw.vertices);
                impl->destroyBuffer(draw.indices);
                throw;
            }
        };
        auto recordDraw = [&](const Prepared& draw) {
            impl->colorWrite = draw.colorWrite;
            impl->api.vkCmdBindPipeline(impl->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        draw.pipeline);
            const VkDeviceSize offset = 0;
            impl->api.vkCmdBindVertexBuffers(impl->commandBuffer, 0, 1, &draw.vertices.buffer,
                                             &offset);
            impl->api.vkCmdPushConstants(impl->commandBuffer, impl->pipelineLayout,
                                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                         0, sizeof(PushConstants), &draw.constants);
            if (draw.indexed) {
                impl->api.vkCmdBindIndexBuffer(impl->commandBuffer, draw.indices.buffer, 0,
                                               VK_INDEX_TYPE_UINT32);
                impl->api.vkCmdDrawIndexed(impl->commandBuffer, draw.count, 1, draw.start, 0, 0);
            } else
                impl->api.vkCmdDraw(impl->commandBuffer, draw.count, 1, draw.start, 0);
            ++impl->info.calls;
            if (draw.wireframe)
                impl->info.lines += draw.count / 2;
            else
                impl->info.triangles += draw.count / 3;
        };
        auto prepareList = [&](const std::vector<Item>& list) {
            const auto overrideMaterial = scene.overrideMaterial;
            for (const auto& item : list) {
                if (!item.mesh->layers.test(camera.layers))
                    continue;
                const auto material = overrideMaterial && item.material->allowOverride
                                          ? overrideMaterial
                                          : item.material;
                auto* group = item.group.get();
                if (item.mesh->onBeforeRender)
                    item.mesh->onBeforeRender(*this, scene, camera, item.geometry.get(),
                                              material.get(), group);
                item.mesh->modelViewMatrix.multiplyMatrices(camera.matrixWorldInverse,
                                                            item.mesh->matrixWorld);
                item.mesh->normalMatrix.getNormalMatrix(item.mesh->modelViewMatrix);
                if (material->onBeforeRender)
                    material->onBeforeRender(*this, scene, camera, item.geometry.get(),
                                             item.mesh.get(), group);
                impl->colorWrite = material->colorWrite;
                const auto drawStart = prepared.size();
                if (material->transparent && material->side == 2 && !material->forceSinglePass) {
                    material->needsUpdate = true;
                    prepare(item, material, 1);
                    material->needsUpdate = true;
                    prepare(item, material, 0);
                } else
                    prepare(item, material, material->side);
                for (std::size_t i = drawStart; i < prepared.size(); ++i)
                    recordDraw(prepared[i]);
                if (item.mesh->onAfterRender)
                    item.mesh->onAfterRender(*this, scene, camera, item.geometry.get(),
                                             material.get(), group);
            }
        };
        prepareList(opaque);
        prepareList(transparent);
        impl->endFrame();
        cleanup();
        if (scene.onAfterRender)
            scene.onAfterRender(*this, scene, camera, nullptr, nullptr, nullptr);
    } catch (...) {
        impl->abortFrame();
        cleanup();
        throw;
    }
}
