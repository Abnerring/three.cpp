#include "renderers/VulkanRenderer.h"
#include "core/RenderTarget.h"
#include "core/WebGLCubeRenderTarget.h"
#include "cameras/CubeCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include "extras/DataUtils.h"
#include "math/JSNumberMath.h"
#include "cameras/Camera.h"
#include "math/JSNumberMath.h"
#include "core/BufferGeometry.h"
#include "math/JSNumberMath.h"
#include "core/InstancedBufferGeometry.h"
#include "math/JSNumberMath.h"
#include "core/ArraySort.h"
#include "math/JSNumberMath.h"
#include "core/NativeObject.h"
#include "math/JSNumberMath.h"
#include "renderers/webgl/WebGLAttributes.h"
#include "math/JSNumberMath.h"
#include "renderers/webgl/WebGLInfo.h"
#include "math/JSNumberMath.h"
#include "renderers/webgl/WebGLGeometries.h"
#include "math/JSNumberMath.h"
#include "renderers/webgl/WebGLObjects.h"
#include "math/JSNumberMath.h"
#include "renderers/webgl/WebGLBindingStates.h"
#include "math/JSNumberMath.h"
#include "renderers/webgl/WebGLClipping.h"
#include "math/JSNumberMath.h"
#include "renderers/webgl/WebGLProperties.h"
#include "math/JSNumberMath.h"
#include "renderers/vulkan/VulkanBindingCommands.h"
#include "math/JSNumberMath.h"
#include "renderers/vulkan/VulkanBufferCommands.h"
#include "math/JSNumberMath.h"
#include "renderers/vulkan/VulkanShaderModules.h"
#include "math/JSNumberMath.h"
#include "renderers/vulkan/VulkanPipelineLayout.h"
#include "math/JSNumberMath.h"
#include "renderers/vulkan/VulkanUniformBuffer.h"
#include "math/JSNumberMath.h"
#include "materials/MeshBasicMaterial.h"
#include "math/JSNumberMath.h"
#include "math/Frustum.h"
#include "math/JSNumberMath.h"
#include "math/Matrix4.h"
#include "math/JSNumberMath.h"
#include "math/Vector4.h"
#include "math/JSNumberMath.h"
#include "objects/Mesh.h"
#include "math/JSNumberMath.h"
#include "objects/InstancedMesh.h"
#include "math/JSNumberMath.h"
#include "objects/SkinnedMesh.h"
#include "math/JSNumberMath.h"
#include "objects/LOD.h"
#include "math/JSNumberMath.h"
#include "objects/Line.h"
#include "math/JSNumberMath.h"
#include "objects/Points.h"
#include "math/JSNumberMath.h"
#include "objects/Sprite.h"
#include "math/JSNumberMath.h"
#include "scenes/FogExp2.h"
#include "math/JSNumberMath.h"
#include "scenes/Scene.h"
#include "math/JSNumberMath.h"
#include "textures/DataTexture.h"
#include "math/JSNumberMath.h"
#include <bit>

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
#include <cstddef>
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
// A stack camera has no owning shared_ptr. Its clipping view borrows live matrix
// elements only for this render; escaped views fail after either exit path.
struct ClippingCameraView {
    struct Borrow { Camera* camera = nullptr; };
    std::shared_ptr<Borrow> borrow;
    JSValue value;
    ClippingCameraView(Camera& camera, const std::shared_ptr<Object3D>& owner) {
        if (owner) { value = nativeObject(owner); return; }
        borrow = std::make_shared<Borrow>(); borrow->camera = &camera;
        const auto token = JSValue::hostObject(borrow, borrow.get(), "three.ClippingCameraBorrow");
        auto elements = JSValue::object(nullptr), matrix = JSValue::object(nullptr);
        const JSValue::NativeFunction live = [](const JSValue&, const auto&, const auto& captures) -> JSValue {
            const auto state = std::static_pointer_cast<Borrow>(captures[0].hostPayload("three.ClippingCameraBorrow"));
            if (!state || !state->camera) throw JSTypeError("Clipping camera view has expired");
            return captures[1];
        };
        for (unsigned i = 0; i < 16; ++i) {
            JSPropertyDescriptor d; d.enumerable = true; d.configurable = false;
            d.getter = JSValue::function([](const JSValue&, const auto&, const auto& captures) -> JSValue {
                const auto state = std::static_pointer_cast<Borrow>(captures[0].hostPayload("three.ClippingCameraBorrow"));
                if (!state || !state->camera) throw JSTypeError("Clipping camera view has expired");
                return state->camera->matrixWorldInverse.elements()[unsigned(captures[1].number())];
            }, {token, i});
            d.setter = JSValue::function([](const JSValue&, const auto& args, const auto& captures) -> JSValue {
                const auto state = std::static_pointer_cast<Borrow>(captures[0].hostPayload("three.ClippingCameraBorrow"));
                if (!state || !state->camera) throw JSTypeError("Clipping camera view has expired");
                const auto number = (args.empty() ? JSValue() : args[0]).toNumber();
                if (!state->camera) throw JSTypeError("Clipping camera view has expired");
                state->camera->matrixWorldInverse.elements()[unsigned(captures[1].number())] = number;
                return {};
            }, {token, i});
            elements.defineProperty(std::to_string(i), d);
        }
        JSPropertyDescriptor d; d.value = 16; elements.defineProperty("length", d);
        d = {}; d.getter = JSValue::function(live, {token, elements}); matrix.defineProperty("elements", d);
        value = JSValue::object(nullptr);
        d.getter = JSValue::function(live, {token, matrix}); value.defineProperty("matrixWorldInverse", d);
    }
    ~ClippingCameraView() { if (borrow) borrow->camera = nullptr; }
};

double geometryInstanceCount(const std::shared_ptr<BufferGeometry>& geometry) {
    const auto value = nativeGeometry(geometry);
    const bool missingMaximum = value.get("_maxInstanceCount").isUndefined();
    const auto maximum = missingMaximum
                             ? JSValue(std::numeric_limits<double>::infinity())
                             : value.get("_maxInstanceCount");
    const auto requested = value.get("instanceCount");
    const auto requestedNumber = requested.toNumber();
    return MathUtils::min(requestedNumber, maximum.toNumber());
}

void check(VkResult result, const char* operation) {
    if (result != VK_SUCCESS)
        throw std::runtime_error(std::string(operation) + " failed (VkResult " +
                                 std::to_string(result) + ")");
}

#define INSTANCE_FUNCTIONS(X)                                                                      \
    X(vkDestroyInstance)                                                                           \
    X(vkEnumeratePhysicalDevices)                                                                  \
    X(vkGetPhysicalDeviceProperties) X(vkGetPhysicalDeviceMemoryProperties)                        \
        X(vkGetPhysicalDeviceQueueFamilyProperties) X(vkGetPhysicalDeviceFormatProperties) X(vkGetPhysicalDeviceFeatures)         \
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
                                                X(vkQueueSubmit) X(vkWaitForFences) \
    X(vkCreateSampler) X(vkDestroySampler) X(vkCmdCopyBufferToImage) X(vkCmdBlitImage) \
    X(vkCmdCopyImage) X(vkCmdClearColorImage) \
    X(vkCreateDescriptorSetLayout) X(vkDestroyDescriptorSetLayout) \
    X(vkCreateDescriptorPool) X(vkDestroyDescriptorPool) X(vkResetDescriptorPool) \
    X(vkAllocateDescriptorSets) X(vkUpdateDescriptorSets) X(vkCmdBindDescriptorSets)

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
                                                 : JSNumberMath::pow(value, 0.41666) * 1.055 - 0.055);
}

// Texture upload conversion preserves IEEE infinities. DataUtils.toHalfFloat is
// a public three.js utility with a different, saturating conversion contract.
std::uint16_t textureHalfFloat(float value) {
    const auto bits = std::bit_cast<std::uint32_t>(value);
    const auto sign = (bits >> 16) & 0x8000;
    const auto mantissa = bits & 0x7fffff;
    const int exponent = int((bits >> 23) & 255) - 127;
    if (exponent == 128) return std::uint16_t(sign | 0x7c00 | (mantissa >> 13));
    if (exponent > 15) return std::uint16_t(sign | 0x7c00);
    if (exponent < -24) return std::uint16_t(sign);
    if (exponent < -14) return std::uint16_t(sign | ((mantissa | 0x800000) >> (-exponent - 1)));
    return std::uint16_t(sign | (std::uint32_t(exponent + 15) << 10) | (mantissa >> 13));
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

std::shared_ptr<BufferAttribute> ordinaryAttribute(const std::shared_ptr<VertexAttribute>& value) {
    const auto result = std::dynamic_pointer_cast<BufferAttribute>(value);
    if (value && !result)
        throw std::invalid_argument("Vulkan interleaved vertex attributes are not implemented yet");
    return result;
}

std::uint32_t unsignedLong(double value) {
    if (!std::isfinite(value) || value == 0) return 0;
    auto result = std::fmod(std::trunc(value), 4294967296.0);
    if (result < 0) result += 4294967296.0;
    return static_cast<std::uint32_t>(result);
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
    std::unique_ptr<VulkanShaderModules> shaderModules;
    std::unique_ptr<VulkanPipelineLayout> reflectedLayout;
    std::unique_ptr<VulkanUniformBuffer> textureUniformTemplate;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    std::unordered_map<std::string, VkPipeline> pipelines;
    VkDescriptorSetLayout textureLayout = VK_NULL_HANDLE;
    // PMREM builds its own reflected layout, template and shader modules; a second program
    // needs its own set because the basic vertex stage uses a different binding split.
    std::unique_ptr<VulkanShaderModules> pmremModules;
    std::unique_ptr<VulkanPipelineLayout> pmremReflectedLayout;
    VkDescriptorSetLayout pmremTextureLayout = VK_NULL_HANDLE;
    VkPipelineLayout pmremPipelineLayout = VK_NULL_HANDLE;
    VkPipeline pmremPipelineHandle = VK_NULL_HANDLE;
    // The blur program mirrors the equirectangular one; its push block adds sigma.
    std::unique_ptr<VulkanShaderModules> pmremBlurModules;
    std::unique_ptr<VulkanPipelineLayout> pmremBlurReflectedLayout;
    VkDescriptorSetLayout pmremBlurTextureLayout = VK_NULL_HANDLE;
    VkPipelineLayout pmremBlurPipelineLayout = VK_NULL_HANDLE;
    VkPipeline pmremBlurPipelineHandle = VK_NULL_HANDLE;
    VkSampler pmremSampler = VK_NULL_HANDLE;
    std::vector<VkDescriptorPool> descriptorPools;
    std::size_t activeDescriptorPool = 0;
    VkCommandBuffer textureCommandBuffer = VK_NULL_HANDLE;
    VkFence textureFence = VK_NULL_HANDLE;
    bool samplerAnisotropy = false;
    float maxAnisotropy = 1;
    std::uint32_t maxTextureSize = 0;
    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        // A cube view over the same six layers, so the blur can sample it as a cubemap.
        VkImageView cubeView = VK_NULL_HANDLE;
    };
    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize size = 0;
        std::uint64_t usedEpoch = 0;
    } readback;
    // The PMREM box positions, uploaded once at construction.
    Buffer pmremVertices;
    // A GPU target owns its own images and framebuffer. Nothing draws into one yet;
    // this is the storage the per target work will move onto.
    struct GpuTarget {
        Image color, depth;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkImageView faceView = VK_NULL_HANDLE;
        VkFramebuffer faceFramebuffer = VK_NULL_HANDLE;
        std::uint32_t initializedLayers = 0;
        bool initialized = false;
        std::uint32_t width = 0, height = 0, layers = 1;
        // CPU copy of the last frame copied out of this target, so readback is per target.
        std::vector<std::uint8_t> pixels;
    };
    std::unique_ptr<GpuTarget> spareTarget;
    // Mirrors the active target while the draw path is migrated onto it.
    std::unique_ptr<GpuTarget> active;
    // Targets not currently bound keep their storage here, keyed by the render target.
    const RenderTarget* activeKey = nullptr;
    std::unordered_map<const RenderTarget*, std::unique_ptr<GpuTarget>> parkedTargets;
    // Parks the bound target and brings the requested one in, transferring ownership.
    void useTarget(const RenderTarget* key) {
        if (activeKey == key) return;
        if (activeKey) parkedTargets[activeKey] = std::move(active);
        active.reset();
        activeKey = key;
        if (!key) return;
        const auto found = parkedTargets.find(key);
        if (found == parkedTargets.end()) return;
        active = std::move(found->second);
        parkedTargets.erase(found);
    }
    using BufferRef = std::shared_ptr<Buffer>;
    // The command adapter owns the upload buffers independently of the decoded
    // float/uint32 draw buffers. Reads below come from mapped Vulkan memory.
    struct AttributeStorage final : VulkanBufferCommands::Storage {
        Impl& renderer;
        Buffer buffer;
        std::size_t length = 0;
        explicit AttributeStorage(Impl& renderer) : renderer(renderer) {}
        ~AttributeStorage() override {
            if (buffer.buffer) --renderer.info.attributeUploadBuffers;
            renderer.destroyBuffer(buffer);
        }
        void setData(std::span<const std::byte> bytes) override {
            auto replacement = renderer.createBuffer(std::max<std::size_t>(bytes.size(), 1),
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                bytes.empty() ? nullptr : bytes.data());
            if (!buffer.buffer) ++renderer.info.attributeUploadBuffers;
            renderer.destroyBuffer(buffer);
            buffer = replacement;
            length = bytes.size();
        }
        void setSubData(std::size_t offset, std::span<const std::byte> bytes) override {
            if (bytes.empty()) return;
            void* mapped;
            check(renderer.api.vkMapMemory(renderer.device, buffer.memory, 0, buffer.size, 0,
                                          &mapped), "vkMapMemory(attribute upload)");
            std::memcpy(static_cast<std::byte*>(mapped) + offset, bytes.data(), bytes.size());
            renderer.api.vkUnmapMemory(renderer.device, buffer.memory);
        }
        std::vector<std::byte> read() const override {
            std::vector<std::byte> bytes(length);
            if (bytes.empty()) return bytes;
            void* mapped;
            check(renderer.api.vkMapMemory(renderer.device, buffer.memory, 0, buffer.size, 0,
                                          &mapped), "vkMapMemory(attribute readback)");
            std::memcpy(bytes.data(), mapped, bytes.size());
            renderer.api.vkUnmapMemory(renderer.device, buffer.memory);
            return bytes;
        }
        std::size_t size() const override { return length; }
    };
    std::unique_ptr<VulkanBufferCommands> attributeCommands;
    std::unique_ptr<VulkanBindingCommands> bindingCommands;
    std::unique_ptr<WebGLBindingStates> bindingStates;
    std::unordered_map<std::string, JSValue> programs;
    std::unique_ptr<WebGLAttributes> attributes;
    std::unique_ptr<WebGLInfo> drawInfo;
    std::unique_ptr<WebGLGeometries> geometries;
    std::unique_ptr<WebGLObjects> objects;
    std::unique_ptr<WebGLProperties> clippingProperties;
    std::unique_ptr<WebGLClipping> clipping;
    JSValue clippingEnabled = false, localClipping = false;
    JSValue currentMaterialId = -1;
    const Camera* currentCamera = nullptr;
    std::uint64_t currentCameraId = 0;
    JSValue currentCameraOwner;
    struct ClippingMaterial {
        std::weak_ptr<Material> owner;
        EventDispatcher::Listener listener;
        JSWeakValue properties;
    };
    std::unordered_map<std::uint64_t, std::shared_ptr<ClippingMaterial>> clippingMaterials;
    struct Lifecycle {
        Impl* owner = nullptr;
    };
    std::shared_ptr<Lifecycle> lifecycle = std::make_shared<Lifecycle>();
    JSValue managedAttributes, managedBindings;
    struct TextureConfig {
        std::uint32_t width = 1, height = 1, levels = 1, components = 4, componentBytes = 1;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        double type = UnsignedByteType;
        VkFilter mag = VK_FILTER_NEAREST, min = VK_FILTER_NEAREST;
        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        VkSamplerAddressMode wrapS = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VkSamplerAddressMode wrapT = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        float anisotropy = 1;
        bool mipFilter = false, generateMipmaps = false, flipY = false, premultiplyAlpha = false;
        std::uint32_t unpackAlignment = 1;
    };
    struct TextureResource {
        Image image;
        VkSampler sampler = VK_NULL_HANDLE;
        TextureConfig config;
    };
    using TextureRef = std::shared_ptr<TextureResource>;
    struct TextureUpload {
        std::uint32_t level, width, height;
        std::vector<std::byte> pixels;
    };
    struct TextureEntry {
        TextureRef resource;
        std::vector<std::byte> pixels;
        bool dataInitialized = false;
        std::size_t usedTimes = 0;
        bool memoryTracked = false;
    };
    struct SourceCache {
        std::weak_ptr<TextureSource> owner;
        std::optional<double> version;
        std::unordered_map<std::string, std::weak_ptr<TextureEntry>> entries;
    };
    struct TextureCache {
        std::weak_ptr<Texture> owner;
        EventDispatcher::Listener disposeListener;
        std::optional<double> version;
        std::shared_ptr<SourceCache> source;
        std::shared_ptr<TextureEntry> entry;
        std::string key;
        bool entryRetired = false;
    };
    std::unordered_map<std::uint64_t, std::shared_ptr<TextureCache>> textureCache;
    std::unordered_map<std::uint64_t, std::shared_ptr<SourceCache>> sourceCache;
    TextureRef whiteTexture, emptyTexture;
    BufferRef zeroUv;
    struct TextureUniforms {
        float mapTransform[12]{}, alphaTransform[12]{}, modelView[16]{};
        // enabled, color enabled, matrix divisor, color divisor
        std::uint32_t instance[4]{};
        // matrix stride, color stride, morph target count, morph vertex stride
        std::uint32_t instanceLayout[4]{};
        std::uint32_t matrixDivisors[4]{1,1,1,1};
        std::uint32_t inputLayout[4][4]{};
        std::uint32_t matrixEnabled[4]{1,1,1,1};
    };
    VulkanUniformBuffer encodeTextureUniforms(const TextureUniforms& snapshot) const {
        auto encoded = *textureUniformTemplate;
        const auto matrix = [](const float* padded) {
            std::array<float, 9> packed{};
            for (unsigned column = 0; column < 3; ++column)
                for (unsigned row = 0; row < 3; ++row)
                    packed[column * 3 + row] = padded[column * 4 + row];
            return packed;
        };
        const auto map = matrix(snapshot.mapTransform), alpha = matrix(snapshot.alphaTransform);
        encoded.write("mapTransform", std::span<const float>(map));
        encoded.write("alphaTransform", std::span<const float>(alpha));
        encoded.write("modelView", std::span<const float>(snapshot.modelView));
        encoded.write("instance", std::span<const std::uint32_t>(snapshot.instance));
        encoded.write("instanceLayout", std::span<const std::uint32_t>(snapshot.instanceLayout));
        encoded.write("matrixDivisors", std::span<const std::uint32_t>(snapshot.matrixDivisors));
        encoded.writeRaw("inputLayout", VulkanUniformBuffer::Scalar::UnsignedInteger, 32,
                         std::as_bytes(std::span(snapshot.inputLayout)));
        encoded.write("matrixEnabled", std::span<const std::uint32_t>(snapshot.matrixEnabled));
        return encoded;
    }
    struct AttributeSnapshot {
        BufferAttribute::Array array;
        double itemSize, count;
        bool normalized, isFloat16;
        double componentStride = 0, componentOffset = 0, pointerSize = 0;
        bool boundView = false, constantInput = false;
        explicit AttributeSnapshot(const BufferAttribute& attribute)
            : array(std::visit([](const auto& values) -> BufferAttribute::Array {
                  return values.slice();
              }, attribute.array)), itemSize(attribute.itemSize), count(attribute.count),
              normalized(attribute.normalized), isFloat16(attribute.isFloat16BufferAttribute.toBoolean()) {}
        AttributeSnapshot(BufferAttribute::Array array, const BufferAttribute& attribute, bool half)
            : array(std::move(array)), itemSize(attribute.itemSize), count(attribute.count),
              normalized(attribute.normalized), isFloat16(half) {}
        double getComponent(double index, double component) const {
            if (boundView && component >= pointerSize) return component == 3 ? 1.0 : 0.0;
            const auto offset = (constantInput ? 0 : index) * (boundView ? componentStride : itemSize) + component + componentOffset;
            return std::visit([&](const auto& values) -> double {
                using Component = typename std::decay_t<decltype(values)>::value_type;
                const auto raw = values.get(offset);
                double value = raw.isUndefined() ? std::numeric_limits<double>::quiet_NaN() : raw.number();
                // HALF_FLOAT vertex fetch ignores GL's normalized flag.
                if constexpr (std::is_same_v<Component, std::uint16_t>)
                    if (isFloat16) return fromHalfFloat(value);
                if constexpr (std::is_same_v<Component, double>) {
                    if (normalized)
                        throw std::invalid_argument("THREE.MathUtils: Invalid component type.");
                }
                if constexpr (std::is_integral_v<Component>)
                    if (normalized)
                        value = std::max(value / static_cast<double>(std::numeric_limits<Component>::max()), -1.0);
                return value;
            }, array);
        }
        AttributeSnapshot(BufferAttribute::Array values, const VulkanBindingCommands::Attribute& pointer, unsigned outputSize)
            : array(std::move(values)), itemSize(outputSize), count(0), normalized(pointer.normalized),
              isFloat16(pointer.type == 0x140b), pointerSize(pointer.size), boundView(true) {
            const auto width = pointer.type == 0x1400 || pointer.type == 0x1401 ? 1u
                : pointer.type == 0x1402 || pointer.type == 0x1403 || pointer.type == 0x140b ? 2u : 4u;
            componentStride = double(pointer.stride ? pointer.stride : pointer.size * width) / width;
            componentOffset = double(pointer.offset) / width;
            const auto length = std::visit([](const auto& v) { return v.size(); }, array);
            count = length < componentOffset + pointerSize ? 0
                : std::floor((double(length) - componentOffset - pointerSize) / componentStride) + 1;
        }
        double getX(std::size_t index) const { return getComponent(index, 0); }
        double getY(std::size_t index) const { return getComponent(index, 1); }
        double getZ(std::size_t index) const { return getComponent(index, 2); }
    };
    struct AttributeCache {
        std::weak_ptr<BufferAttribute> owner;
        std::shared_ptr<AttributeSnapshot> snapshot;
        EventDispatcher::Listener disposeListener;
        std::array<BufferRef, 2> vertices;
        BufferRef indices;
        JSWeakValue key;
        JSWeakValue data;
        JSWeakValue boundBuffer;
        std::uint64_t revision = std::numeric_limits<std::uint64_t>::max();
    };
    struct GeometryCache {
        std::weak_ptr<BufferGeometry> owner;
        std::vector<AttributeSnapshot> morphPositions;
        bool morphInitialized = false;
        BufferRef morphBuffer;
        std::uint32_t morphVertexCount = 0;
    };
    struct InstanceCache {
        struct Binding {
            std::vector<JSWeakValue> attributes;
            std::uint32_t matrixStride = 16, colorStride = 3, matrixDivisor = 1, colorDivisor = 1;
            std::array<std::uint32_t,4> matrixDivisors{1,1,1,1};
            bool matrixNormalized = false, colorNormalized = false;
            bool matrixEnabled = false, colorEnabled = false;
        };
        std::weak_ptr<InstancedMesh> owner;
        std::unordered_map<std::string, std::shared_ptr<Binding>> bindings;
    };
    std::unordered_map<std::uint64_t, std::shared_ptr<AttributeCache>> attributeCache;
    std::unordered_map<std::uint64_t, std::shared_ptr<GeometryCache>> geometryCache;
    std::unordered_map<std::uint64_t, std::shared_ptr<InstanceCache>> instanceCache;
    struct BoundInput {
        JSWeakValue buffer;
        VulkanBindingCommands::Attribute pointer;
        unsigned outputSize = 0;
        std::shared_ptr<AttributeCache> cached;
    };
    std::vector<BoundInput> boundInputs;
    struct MatrixInput {
        std::array<JSWeakValue,4> buffers;
        std::array<VulkanBindingCommands::Attribute,4> pointers;
        std::array<std::uint64_t,4> revisions{};
        std::shared_ptr<AttributeCache> cached;
    };
    std::vector<MatrixInput> matrixInputs;
    BufferRef whiteColor, noFog, emptyStorage;
    std::uint64_t recordingEpoch = 0;
    std::uint32_t maxVertexStride = 0;
    VkDeviceSize maxStorageBufferRange = 0;

    VkFramebuffer faceFramebuffer = VK_NULL_HANDLE;
    VkImage faceImage = VK_NULL_HANDLE;
    std::uint32_t width = 0, height = 0;

    bool recording = false;
    bool rendering = false;
    std::shared_ptr<RenderTarget> renderTarget;
    std::uint32_t activeCubeFace = 0, activeMipmapLevel = 0;
    std::uint32_t savedWidth = 0, savedHeight = 0;
    bool disposing = false;
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
                    maxVertexStride = properties.limits.maxVertexInputBindingStride;
                    maxStorageBufferRange = properties.limits.maxStorageBufferRange;
                    maxTextureSize = properties.limits.maxImageDimension2D;
                    maxAnisotropy = properties.limits.maxSamplerAnisotropy;
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
        VkPhysicalDeviceFeatures supported{};
        api.vkGetPhysicalDeviceFeatures(physicalDevice, &supported);
        VkPhysicalDeviceFeatures enabled{};
        enabled.samplerAnisotropy = supported.samplerAnisotropy;
        samplerAnisotropy = supported.samplerAnisotropy == VK_TRUE;
        VkDeviceCreateInfo deviceCreate{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceCreate.pEnabledFeatures = &enabled;
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
        check(api.vkAllocateCommandBuffers(device, &commandAllocate, &textureCommandBuffer),
              "vkAllocateCommandBuffers(texture)");
        check(api.vkCreateFence(device, &fenceCreate, nullptr, &textureFence), "vkCreateFence(texture)");
        const auto vertexWords = readShader("basic.vert.spv");
        const auto fragmentWords = readShader("basic.frag.spv");
        const std::array<VulkanShaderModules::Stage, 2> compiledStages{{{vertexWords}, {fragmentWords}}};
        shaderModules = std::make_unique<VulkanShaderModules>(device,
            VulkanShaderModules::Dispatch{api.vkCreateShaderModule, api.vkDestroyShaderModule}, compiledStages);
        VkPhysicalDeviceProperties shaderProperties{};
        api.vkGetPhysicalDeviceProperties(physicalDevice, &shaderProperties);
        const auto& vertexInterface = shaderModules->interfaces().front();
        const auto transforms = std::find_if(vertexInterface.descriptors.begin(), vertexInterface.descriptors.end(),
            [](const auto& descriptor) { return descriptor.set == 0 && descriptor.binding == 2; });
        if (transforms == vertexInterface.descriptors.end() ||
            transforms->type != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || transforms->count != 1)
            throw std::runtime_error("MeshBasic shader requires one reflected texture uniform block");
        textureUniformTemplate = std::make_unique<VulkanUniformBuffer>(transforms->block);
        if (textureUniformTemplate->bytes().empty() ||
            textureUniformTemplate->bytes().size() > shaderProperties.limits.maxUniformBufferRange)
            throw std::runtime_error("Texture uniform block exceeds Vulkan device range limits");
        reflectedLayout = std::make_unique<VulkanPipelineLayout>(device,
            VulkanPipelineLayout::Dispatch{api.vkCreateDescriptorSetLayout, api.vkDestroyDescriptorSetLayout,
                api.vkCreatePipelineLayout, api.vkDestroyPipelineLayout},
            shaderModules->interfaces(), shaderProperties.limits, VulkanPipelineLayout::DescriptorCounts{}, 1);
        if (reflectedLayout->setLayouts().size() != 1)
            throw std::runtime_error("MeshBasic shader requires one descriptor set");
        textureLayout = reflectedLayout->setLayouts()[0];
        // Second program: the PMREM equirectangular pass, same construction path.
        const auto pmremVertexWords = readShader("pmrem_equirect.vert.spv");
        const auto pmremFragmentWords = readShader("pmrem_equirect.frag.spv");
        const std::array<VulkanShaderModules::Stage, 2> pmremStages{{{pmremVertexWords}, {pmremFragmentWords}}};
        pmremModules = std::make_unique<VulkanShaderModules>(device,
            VulkanShaderModules::Dispatch{api.vkCreateShaderModule, api.vkDestroyShaderModule}, pmremStages);
        pmremReflectedLayout = std::make_unique<VulkanPipelineLayout>(device,
            VulkanPipelineLayout::Dispatch{api.vkCreateDescriptorSetLayout, api.vkDestroyDescriptorSetLayout,
                api.vkCreatePipelineLayout, api.vkDestroyPipelineLayout},
            pmremModules->interfaces(), shaderProperties.limits, VulkanPipelineLayout::DescriptorCounts{}, 1);
        if (pmremReflectedLayout->setLayouts().size() != 1)
            throw std::runtime_error("PMREM shader requires one descriptor set");
        pmremTextureLayout = pmremReflectedLayout->setLayouts()[0];
        pmremPipelineLayout = pmremReflectedLayout->handle();
        createPmremGeometry();
        {
            VkSamplerCreateInfo pmremSamplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            pmremSamplerInfo.magFilter = VK_FILTER_LINEAR;
            pmremSamplerInfo.minFilter = VK_FILTER_LINEAR;
            pmremSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            pmremSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            pmremSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            pmremSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            check(api.vkCreateSampler(device, &pmremSamplerInfo, nullptr, &pmremSampler),
                  "vkCreateSampler(pmrem)");
        }
        // Exercise the per target builder once so it cannot rot unnoticed.
        spareTarget = createGpuTarget(4, 4, 1);
        createPmremPipeline();
        // Blur program: same construction path, its own reflected layout.
        const auto blurVertexWords = readShader("pmrem_blur.vert.spv");
        const auto blurFragmentWords = readShader("pmrem_blur.frag.spv");
        const std::array<VulkanShaderModules::Stage, 2> blurStages{{{blurVertexWords}, {blurFragmentWords}}};
        pmremBlurModules = std::make_unique<VulkanShaderModules>(device,
            VulkanShaderModules::Dispatch{api.vkCreateShaderModule, api.vkDestroyShaderModule}, blurStages);
        pmremBlurReflectedLayout = std::make_unique<VulkanPipelineLayout>(device,
            VulkanPipelineLayout::Dispatch{api.vkCreateDescriptorSetLayout, api.vkDestroyDescriptorSetLayout,
                api.vkCreatePipelineLayout, api.vkDestroyPipelineLayout},
            pmremBlurModules->interfaces(), shaderProperties.limits, VulkanPipelineLayout::DescriptorCounts{}, 1);
        if (pmremBlurReflectedLayout->setLayouts().size() != 1)
            throw std::runtime_error("PMREM blur shader requires one descriptor set");
        pmremBlurTextureLayout = pmremBlurReflectedLayout->setLayouts()[0];
        pmremBlurPipelineLayout = pmremBlurReflectedLayout->handle();
        createPmremBlurPipeline();
        pipelineLayout = reflectedLayout->handle();
        createRenderPass();
        resize(parameters.width, parameters.height);
        attributeCommands = std::make_unique<VulkanBufferCommands>([this] {
            return std::make_shared<AttributeStorage>(*this);
        });
        VkPhysicalDeviceProperties inputProperties{};
        api.vkGetPhysicalDeviceProperties(physicalDevice, &inputProperties);
        bindingCommands = std::make_unique<VulkanBindingCommands>(attributeCommands->object(), attributeCommands->bindingInterface(),
            inputProperties.limits.maxVertexInputAttributes);
        attributes = std::make_unique<WebGLAttributes>(bindingCommands->object());
    }

    void initializeObjectProtocols() {
        lifecycle->owner = this;
        const auto token = JSValue::hostObject(lifecycle, lifecycle.get(), "VulkanRenderer.Lifecycle");
        managedAttributes = JSValue::object();
        managedBindings = JSValue::object();
        const auto define = [](JSValue target, const char* name, unsigned arity,
                               JSValue::NativeFunction callback, std::vector<JSValue> captures) {
            JSPropertyDescriptor descriptor;
            descriptor.value = JSValue::constructor(callback, captures, name, arity,
                                                    JSValue::ConstructorKind::Callable);
            descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
            target.defineProperty(name, descriptor);
        };
        for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
                 {"get", 1}, {"remove", 1}, {"update", 2}}) {
            define(managedAttributes, name, arity,
                +[](const JSValue&, const std::vector<JSValue>& args,
                    const std::vector<JSValue>& captures)->JSValue {
                    const auto lifetime = std::static_pointer_cast<Lifecycle>(
                        captures[0].hostPayload("VulkanRenderer.Lifecycle"));
                    if (!lifetime->owner) return {};
                    const auto raw = captures[1], method = raw.get(captures[2].string());
                    const auto result = method.call(raw, args);
                    // Raw protocol calls complete before any native bookkeeping.
                    // Neither tracking nor decoding executes another update/getter.
                    if (lifetime->owner && !args.empty()) {
                        if (captures[2].strictEquals("update")) lifetime->owner->trackAttribute(args[0]);
                        else if (captures[2].strictEquals("remove")) lifetime->owner->dropAttributeKey(args[0]);
                    }
                    return result;
                }, {token, attributes->object(), name});
        }
        bindingStates = std::make_unique<WebGLBindingStates>(bindingCommands->object(), managedAttributes);
        for (const char* name : {"releaseStatesOfGeometry", "releaseStatesOfObject"}) {
            define(managedBindings, name, 1,
                +[](const JSValue&, const std::vector<JSValue>& args,
                    const std::vector<JSValue>& captures)->JSValue {
                    const auto lifetime = std::static_pointer_cast<Lifecycle>(
                        captures[0].hostPayload("VulkanRenderer.Lifecycle"));
                    if (!lifetime->owner) return {};
                    const auto value = args.empty() ? JSValue{} : args[0];
                    const auto bindings = lifetime->owner->bindingStates->object();
                    bindings.get(captures[1].string()).call(bindings,{value});
                    if (!lifetime->owner) return {};
                    if (captures[1].strictEquals("releaseStatesOfGeometry")) {
                        const auto geometry = std::static_pointer_cast<BufferGeometry>(
                            value.hostPayload("three.BufferGeometry"));
                        lifetime->owner->removeGeometry(geometry->id);
                    } else {
                        const auto object = std::static_pointer_cast<Object3D>(value.hostPayload("three.Object3D"));
                        lifetime->owner->removeInstance(object->id);
                    }
                    return {};
                }, {token, name});
        }
        // The actual binding module and command state feed Vulkan draw preparation.
        geometries = std::make_unique<WebGLGeometries>(bindingCommands->object(), managedAttributes,
                                                      drawInfo->object(), managedBindings);
        objects = std::make_unique<WebGLObjects>(bindingCommands->object(), geometries->object(),
                                                managedAttributes, managedBindings, drawInfo->object());
    }

    ~Impl() {
        disposing = true;
        // Escaped listeners and managed closures may survive this renderer.
        // Retire their native capability before cleanup can invoke user code.
        lifecycle->owner = nullptr;
        for (const auto& [id, cached] : clippingMaterials)
            if (const auto material = cached->owner.lock()) material->removeEventListener("dispose", cached->listener);
        clippingMaterials.clear();
        if (device && api.vkDeviceWaitIdle)
            api.vkDeviceWaitIdle(device);
        if (device) {
            // Native destruction cannot propagate managed callback exceptions.
            // Every escaped interface remains inert even if disposal is interrupted.
            try { if (objects) objects->dispose(); if (geometries) geometries->dispose(); } catch (...) {}
            try { if (bindingStates) bindingStates->dispose(); } catch (...) {}
            boundInputs.clear(); matrixInputs.clear(); programs.clear();
            while (!instanceCache.empty()) removeInstance(instanceCache.begin()->first, true);
            while (!geometryCache.empty())
                removeGeometry(geometryCache.begin()->first);
            while (!attributeCache.empty())
                removeAttribute(attributeCache.begin()->first);
            // Managed wrappers may outlive this renderer. Retire every native
            // upload resource while its device and function table still exist.
            if (bindingCommands) bindingCommands->shutdown();
            if (attributeCommands) attributeCommands->shutdown();
            bindingStates.reset(); bindingCommands.reset();
            objects.reset();
            geometries.reset();
            managedBindings = {};
            managedAttributes = {};
            attributes.reset();
            attributeCommands.reset();
            while (!textureCache.empty()) removeTexture(textureCache.begin()->first);
            sourceCache.clear();
            whiteTexture.reset();
            emptyTexture.reset();
            zeroUv.reset();
            for (const auto pool : descriptorPools) api.vkDestroyDescriptorPool(device, pool, nullptr);
            if (textureFence) api.vkDestroyFence(device, textureFence, nullptr);
            whiteColor.reset();
            noFog.reset();
            emptyStorage.reset();
            destroyTargets();
            for (const auto& entry : pipelines)
                api.vkDestroyPipeline(device, entry.second, nullptr);
            reflectedLayout.reset();
            shaderModules.reset();
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
    BufferRef sharedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, const void* data,
                           bool attribute = false) {
        auto result = BufferRef(new Buffer{}, [this, attribute](Buffer* buffer) {
            destroyBuffer(*buffer);
            delete buffer;
            if (attribute)
                --info.attributeBuffers;
        });
        if (attribute)
            ++info.attributeBuffers;
        *result = createBuffer(size, usage, data);
        return result;
    }
    template <class Attribute> static std::size_t componentCount(const Attribute& attribute) {
        return std::visit([](const auto& values) { return values.size(); }, attribute.array);
    }
    static std::vector<float> vertexValues(AttributeSnapshot& snapshot, bool normalized) {
        if (!std::isfinite(snapshot.itemSize) || snapshot.itemSize <= 0 ||
            std::trunc(snapshot.itemSize) != snapshot.itemSize)
            throw std::invalid_argument("Vulkan vertex attribute itemSize must be a positive integer");
        snapshot.normalized = normalized;
        std::vector<float> values(snapshot.boundView ? std::size_t(snapshot.count) * std::size_t(snapshot.itemSize) : componentCount(snapshot));
        if(snapshot.boundView) {
            for(std::size_t i=0;i<values.size();++i)
                values[i]=float(snapshot.getComponent(double(i/std::size_t(snapshot.itemSize)),double(i%std::size_t(snapshot.itemSize))));
            return values;
        }
        for (std::size_t i = 0; i < values.size(); ++i)
            values[i] = static_cast<float>(snapshot.getComponent(0, double(i)));
        return values;
    }
    template <class Attribute> static std::vector<std::uint32_t> indexValues(const Attribute& snapshot) {
        return std::visit(
            [](const auto& values) -> std::vector<std::uint32_t> {
                using Component = typename std::decay_t<decltype(values)>::value_type;
                if constexpr (std::is_same_v<Component, std::uint8_t> ||
                              std::is_same_v<Component, std::uint16_t> ||
                              std::is_same_v<Component, std::uint32_t>)
                    return {values.begin(), values.end()};
                else
                    throw std::invalid_argument(
                        "Mesh index array must use unsigned 8, 16 or 32-bit integers");
            }, snapshot.array);
    }
    struct ComponentRange { std::size_t start, count; };
    template <class T>
    void updateBuffer(BufferRef& buffer, const std::vector<T>& values,
                      const std::vector<ComponentRange>& ranges,
                      VkBufferUsageFlags usage) {
        if (!buffer)
            return;
        if (recording && buffer->usedEpoch == recordingEpoch) {
            // Earlier draws in this command buffer must retain their pre-update data.
            buffer = sharedBuffer(std::max<std::size_t>(values.size(), 1) * sizeof(T), usage,
                                  values.empty() ? nullptr : values.data(), true);
            return;
        }
        void* mapped;
        check(api.vkMapMemory(device, buffer->memory, 0, buffer->size, 0, &mapped), "vkMapMemory");
        for (const auto& range : ranges)
            if (range.count)
                std::memcpy(static_cast<std::byte*>(mapped) + range.start * sizeof(T),
                            values.data() + range.start, range.count * sizeof(T));
        api.vkUnmapMemory(device, buffer->memory);
    }
    void removeAttribute(std::uint64_t id, bool removeRaw = true) {
        const auto found = attributeCache.find(id);
        if (found == attributeCache.end())
            return;
        const auto cached = found->second;
        if (const auto owner = cached->owner.lock())
            owner->removeEventListener("dispose", cached->disposeListener);
        attributeCache.erase(found);
        if (const auto key = cached->key.lock(); !disposing && removeRaw && key && attributes)
            managedAttributes.get("remove").call(managedAttributes, {*key});
    }
    void dropAttributeKey(const JSValue& key) {
        pruneBoundInputs();
        std::vector<std::uint64_t> ids;
        for (const auto& [id, cached] : attributeCache)
            if (const auto candidate = cached->key.lock(); candidate && candidate->sameValue(key))
                ids.push_back(id);
        for (const auto id : ids) {
            const auto found = attributeCache.find(id);
            if (found == attributeCache.end()) continue;
            const auto cached = found->second;
            if (const auto owner = cached->owner.lock())
                owner->removeEventListener("dispose", cached->disposeListener);
            attributeCache.erase(found);
        }
    }
    void adjustMemory(const char* name, double change) {
        if (disposing || !drawInfo) return;
        auto memory = drawInfo->object().get("memory");
        memory.set(name, memory.get(name).toNumber() + change);
    }
    void removeGeometry(std::uint64_t id) {
        pruneBoundInputs();
        const auto found = geometryCache.find(id);
        if (found == geometryCache.end()) return;
        geometryCache.erase(found);
        const auto prefix = std::to_string(id) + ":";
        for (auto& [objectId, instance] : instanceCache)
            for (auto binding = instance->bindings.begin(); binding != instance->bindings.end();)
                if (binding->first.starts_with(prefix)) binding = instance->bindings.erase(binding);
                else ++binding;
        // WebGLGeometries alone owns raw removal, listeners and memory accounting.
        // This adapter retires only derived Vulkan binding/deformation storage.
    }
    void pruneBoundInputs() {
        std::erase_if(boundInputs, [&](const auto& input) {
            const auto key=input.buffer.lock();return !key || !attributeCommands->hasStorage(*key);
        });
        std::erase_if(matrixInputs, [&](const auto& input) {
            for(unsigned i=0;i<4;++i) if(input.pointers[i].enabled) {
                const auto key=input.buffers[i].lock();if(!key || !attributeCommands->hasStorage(*key))return true;
            }
            return false;
        });
    }
    void pruneResources() {
        pruneBoundInputs();
        std::erase_if(clippingMaterials, [](const auto& item) { return item.second->owner.expired(); });
        std::vector<std::uint64_t> expired;
        for (const auto& [id, cached] : instanceCache)
            if (cached->owner.expired()) expired.push_back(id);
        for (const auto id : expired) removeInstance(id, true);
        expired.clear();
        for (const auto& [id, cached] : geometryCache)
            if (cached->owner.expired())
                expired.push_back(id);
        for (const auto id : expired)
            removeGeometry(id);
        expired.clear();
        for (const auto& [id, cached] : attributeCache)
            if (cached->owner.expired())
                expired.push_back(id);
        // The managed attribute may outlive a native handle. Its WeakMap entry
        // remains authoritative when a new handle later wraps that same object.
        // Explicit attribute/geometry disposal still removes the managed entry.
        for (const auto id : expired)
            attributeCache.erase(id);
        expired.clear();
        for (const auto& [id, cached] : textureCache)
            if (cached->owner.expired()) expired.push_back(id);
        for (const auto id : expired) removeTexture(id);
        for (auto it = sourceCache.begin(); it != sourceCache.end();) {
            if (it->second->owner.expired()) it = sourceCache.erase(it);
            else {
                for (auto entry = it->second->entries.begin(); entry != it->second->entries.end();) {
                    if (entry->second.expired()) entry = it->second->entries.erase(entry);
                    else ++entry;
                }
                ++it;
            }
        }
    }
    void removeInstance(std::uint64_t id, bool forget = false) {
        const auto found = instanceCache.find(id);
        if (found == instanceCache.end()) return;
        const auto cached = found->second;
        cached->bindings.clear();
        if (forget) instanceCache.erase(found);
    }
    void registerInstance(const std::shared_ptr<InstancedMesh>& mesh) {
        auto found = instanceCache.find(mesh->id);
        if (found == instanceCache.end()) {
            auto entry = std::make_shared<InstanceCache>();
            entry->owner = mesh;
            found = instanceCache.emplace(mesh->id, std::move(entry)).first;
        }
    }
    void removeTexture(std::uint64_t id) {
        const auto found = textureCache.find(id);
        if (found == textureCache.end()) return;
        const auto cached = found->second;
        if (const auto owner = cached->owner.lock())
            owner->removeEventListener("dispose", cached->disposeListener);
        textureCache.erase(found);
        releaseTextureEntry(cached);
    }
    void releaseTextureEntry(const std::shared_ptr<TextureCache>& cached) {
        const auto entry = cached->entry;
        if (!entry || cached->entryRetired) return;
        cached->entryRetired = true;
        if (--entry->usedTimes != 0) return;
        // Retire the logical source/cache-key storage before observable memory
        // conversion. Recorded draws can still retain their physical VkImage.
        if (cached->source) {
            const auto found = cached->source->entries.find(cached->key);
            if (found != cached->source->entries.end() && found->second.lock() == entry)
                cached->source->entries.erase(found);
        }
        if (entry->memoryTracked) {
            entry->memoryTracked = false;
            adjustMemory("textures", -1);
        }
    }
    static std::string textureKey(const Texture& texture) {
        std::string key;
        for (double value : {texture.wrapS, texture.wrapT, 0.0, texture.magFilter,
                             texture.minFilter, texture.anisotropy})
            key += JSValue(value).toString() + ',';
        key += texture.internalFormat.value_or("") + ',';
        for (double value : {texture.format, texture.type}) key += JSValue(value).toString() + ',';
        for (bool value : {texture.generateMipmaps, texture.mipmapsAutoUpdate,
                           texture.premultiplyAlpha, texture.flipY})
            key += value ? "true," : "false,";
        return key + JSValue(texture.unpackAlignment).toString() + ',' + texture.colorSpace;
    }
    TextureConfig textureConfig(const Texture& texture, const TextureImage& image) {
        TextureConfig config;
        if (!texture.isDataTexture || texture.isDataArrayTexture || texture.isData3DTexture ||
            texture.isCubeTexture || texture.isCompressedTexture || texture.isDepthTexture ||
            texture.isRenderTargetTexture)
            throw std::invalid_argument("Vulkan sampling currently requires a 2D DataTexture");
        const auto dimension = [&](double value, bool allowZero = false) {
            if (!std::isfinite(value) || value < (allowZero ? 0 : 1) || value > maxTextureSize || std::trunc(value) != value)
                throw std::invalid_argument("Texture dimensions exceed Vulkan limits or are not positive integers");
            return static_cast<std::uint32_t>(value);
        };
        config.width = dimension(image.width); config.height = dimension(image.height);
        const auto generatedLevels = std::uint32_t(std::floor(std::log2(std::max(config.width, config.height)))) + 1;
        const bool manual = texture.mipmaps && !texture.mipmaps->empty();
        if (manual) {
            const auto& base = texture.mipmaps->front();
            if (!base) throw std::invalid_argument("DataTexture base mipmap is null");
            config.width = dimension(base->width, true); config.height = dimension(base->height, true);
        }
        config.components = texture.format == RGBAFormat ? 4 : texture.format == RGFormat ? 2 :
                            texture.format == RedFormat ? 1 : 0;
        if (!config.components)
            throw std::invalid_argument("Vulkan DataTexture supports RGBA, RG and Red formats");
        config.type = texture.type;
        std::string internal;
        if (texture.type == UnsignedByteType) {
            config.componentBytes = 1;
            config.format = config.components == 4 ? VK_FORMAT_R8G8B8A8_UNORM :
                            config.components == 2 ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R8_UNORM;
            internal = config.components == 4 ? "RGBA8" : config.components == 2 ? "RG8" : "R8";
        } else if (texture.type == FloatType) {
            config.componentBytes = 4;
            config.format = config.components == 4 ? VK_FORMAT_R32G32B32A32_SFLOAT :
                            config.components == 2 ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R32_SFLOAT;
            internal = config.components == 4 ? "RGBA32F" : config.components == 2 ? "RG32F" : "R32F";
        } else if (texture.type == HalfFloatType) {
            config.componentBytes = 2;
            config.format = config.components == 4 ? VK_FORMAT_R16G16B16A16_SFLOAT :
                            config.components == 2 ? VK_FORMAT_R16G16_SFLOAT : VK_FORMAT_R16_SFLOAT;
            internal = config.components == 4 ? "RGBA16F" : config.components == 2 ? "RG16F" : "R16F";
        } else throw std::invalid_argument("Vulkan DataTexture supports UnsignedByte, Float and HalfFloat types");
        if (texture.colorSpace == SRGBColorSpace) {
            if (texture.type != UnsignedByteType || texture.format != RGBAFormat)
                throw std::invalid_argument("sRGB DataTexture requires RGBA UnsignedByte storage");
            config.format = VK_FORMAT_R8G8B8A8_SRGB; internal = "SRGB8_ALPHA8";
        } else if (texture.colorSpace != NoColorSpace && texture.colorSpace != LinearSRGBColorSpace)
            throw std::invalid_argument("Vulkan texture color space is not supported");
        if (texture.internalFormat && *texture.internalFormat != internal)
            throw std::invalid_argument("Vulkan explicit texture internalFormat must match its supported storage format");
        const auto wrap = [](double value) {
            if (value == RepeatWrapping) return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            if (value == ClampToEdgeWrapping) return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            if (value == MirroredRepeatWrapping) return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            throw std::invalid_argument("Unsupported texture wrapping mode");
        };
        config.wrapS = wrap(texture.wrapS); config.wrapT = wrap(texture.wrapT);
        if (texture.magFilter != NearestFilter && texture.magFilter != LinearFilter)
            throw std::invalid_argument("Unsupported texture magnification filter");
        config.mag = texture.magFilter == LinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        if (texture.minFilter == NearestFilter || texture.minFilter == LinearFilter) {
            config.min = texture.minFilter == LinearFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        } else {
            config.mipFilter = true;
            if (texture.minFilter == NearestMipmapNearestFilter || texture.minFilter == NearestMipmapLinearFilter)
                config.min = VK_FILTER_NEAREST;
            else if (texture.minFilter == LinearMipmapNearestFilter || texture.minFilter == LinearMipmapLinearFilter)
                config.min = VK_FILTER_LINEAR;
            else throw std::invalid_argument("Unsupported texture minification filter");
            config.mipmapMode = texture.minFilter == NearestMipmapLinearFilter || texture.minFilter == LinearMipmapLinearFilter
                                    ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }
        // WebGL allocates before the DataTexture manual-mipmap branch turns generation off.
        if (texture.generateMipmaps) config.levels = generatedLevels;
        else if (manual) config.levels = static_cast<std::uint32_t>(texture.mipmaps->size());
        config.generateMipmaps = texture.generateMipmaps && texture.mipmapsAutoUpdate && !manual;
        if (texture.unpackAlignment != 1 && texture.unpackAlignment != 2 &&
            texture.unpackAlignment != 4 && texture.unpackAlignment != 8)
            throw std::invalid_argument("Texture unpackAlignment must be 1, 2, 4 or 8");
        config.unpackAlignment = std::uint32_t(texture.unpackAlignment);
        config.flipY = texture.flipY; config.premultiplyAlpha = texture.premultiplyAlpha;
        if (samplerAnisotropy && config.mag == VK_FILTER_LINEAR && config.mipFilter &&
            config.mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR && texture.anisotropy > 1) {
            config.anisotropy = float(std::min(texture.anisotropy, double(maxAnisotropy)));
        }
        VkFormatProperties properties{};
        api.vkGetPhysicalDeviceFormatProperties(physicalDevice, config.format, &properties);
        VkFormatFeatureFlags required = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
                                       VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        if (config.mag == VK_FILTER_LINEAR || config.min == VK_FILTER_LINEAR ||
            config.mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR || (config.generateMipmaps && config.levels > 1))
            required |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if (config.generateMipmaps && config.levels > 1) required |= VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
        if ((properties.optimalTilingFeatures & required) != required)
            throw std::invalid_argument("The Vulkan device does not support the requested texture format/filter/mipmap combination");
        return config;
    }
    static std::vector<std::byte> texturePixels(const TextureImage& image, const TextureConfig& config) {
        const std::size_t pixelBytes = config.components * config.componentBytes;
        const std::size_t rowBytes = std::size_t(config.width) * pixelBytes;
        const std::size_t sourceStride = (rowBytes + config.unpackAlignment - 1) / config.unpackAlignment * config.unpackAlignment;
        std::vector<std::byte> raw;
        std::visit([&](const auto& values) {
            using T = std::decay_t<decltype(values)>;
            if constexpr (std::is_same_v<T, std::nullptr_t> || std::is_same_v<T, JSValue>) {
                throw std::invalid_argument("DataTexture upload requires a native typed pixel array");
            } else {
                using Component = typename T::value_type;
                const bool compatible = (config.type == UnsignedByteType && std::is_same_v<Component, std::uint8_t>) ||
                                        (config.type == FloatType && std::is_same_v<Component, float>) ||
                                        (config.type == HalfFloatType && std::is_same_v<Component, std::uint16_t>);
                if (!compatible) throw std::invalid_argument("DataTexture pixel array does not match texture.type");
                raw.resize(values.size() * sizeof(Component));
                for (std::size_t i = 0; i < values.size(); ++i) {
                    const Component value = values[i];
                    std::memcpy(raw.data() + i * sizeof(Component), &value, sizeof(value));
                }
            }
        }, image.data);
        if (config.width == 0 || config.height == 0) return {};
        if (raw.size() < sourceStride * (config.height - 1) + rowBytes)
            throw std::invalid_argument("DataTexture pixel array is shorter than its dimensions/unpackAlignment");
        std::vector<std::byte> result(rowBytes * config.height);
        for (std::uint32_t row = 0; row < config.height; ++row) {
            const auto sourceRow = config.flipY ? config.height - row - 1 : row;
            std::memcpy(result.data() + row * rowBytes, raw.data() + sourceRow * sourceStride, rowBytes);
        }
        if (config.premultiplyAlpha && config.components == 4) {
            for (std::size_t offset = 0; offset < result.size(); offset += pixelBytes) {
                if (config.type == UnsignedByteType) {
                    const auto alpha = std::to_integer<unsigned>(result[offset + 3]);
                    // Blink's WebGLImageConversion RGBA8 typed-array path truncates
                    // the float product instead of rounding it to the nearest byte.
                    const float scale = float(alpha) / 255.0f;
                    for (int c = 0; c < 3; ++c)
                        result[offset + c] = std::byte(static_cast<std::uint8_t>(float(std::to_integer<unsigned>(result[offset + c])) * scale));
                } else if (config.type == FloatType) {
                    float pixel[4]; std::memcpy(pixel, result.data() + offset, sizeof(pixel));
                    for (int c = 0; c < 3; ++c) pixel[c] *= pixel[3];
                    std::memcpy(result.data() + offset, pixel, sizeof(pixel));
                } else {
                    std::uint16_t pixel[4]; std::memcpy(pixel, result.data() + offset, sizeof(pixel));
                    for (int c = 0; c < 3; ++c) pixel[c] = textureHalfFloat(float(fromHalfFloat(pixel[c])) * float(fromHalfFloat(pixel[3])));
                    std::memcpy(result.data() + offset, pixel, sizeof(pixel));
                }
            }
        }
        return result;
    }
    TextureRef createTextureResource(const TextureConfig& config, const std::vector<TextureUpload>& uploads,
                                    const TextureRef& previous = {}, bool tracked = true) {
        auto result = TextureRef(new TextureResource{}, [this, tracked](TextureResource* texture) {
            if (texture->sampler) api.vkDestroySampler(device, texture->sampler, nullptr);
            destroyImage(texture->image); delete texture;
            if (tracked) --info.textureImages;
        });
        if (tracked) ++info.textureImages;
        result->config = config;
        auto& image = result->image;
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        create.imageType = VK_IMAGE_TYPE_2D; create.format = config.format;
        create.extent = {config.width, config.height, 1}; create.mipLevels = config.levels;
        create.arrayLayers = 1;
        create.tiling = VK_IMAGE_TILING_OPTIMAL;
        create.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        check(api.vkCreateImage(device, &create, nullptr, &image.image), "vkCreateImage(texture)");
        VkMemoryRequirements requirements{}; api.vkGetImageMemoryRequirements(device, image.image, &requirements);
        VkMemoryAllocateInfo allocate{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate.allocationSize = requirements.size;
        allocate.memoryTypeIndex = memoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        check(api.vkAllocateMemory(device, &allocate, nullptr, &image.memory), "vkAllocateMemory(texture)");
        check(api.vkBindImageMemory(device, image.image, image.memory, 0), "vkBindImageMemory(texture)");
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = image.image; view.viewType = VK_IMAGE_VIEW_TYPE_2D; view.format = config.format;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, config.levels, 0, 1};
        check(api.vkCreateImageView(device, &view, nullptr, &image.view), "vkCreateImageView(texture)");
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler.magFilter = config.mag; sampler.minFilter = config.min; sampler.mipmapMode = config.mipmapMode;
        sampler.addressModeU = config.wrapS; sampler.addressModeV = config.wrapT;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.anisotropyEnable = config.anisotropy > 1; sampler.maxAnisotropy = config.anisotropy;
        // Clamp non-mipmap filters to level zero, matching WebGL's minification modes.
        sampler.maxLod = config.mipFilter ? float(config.levels - 1) : 0;
        check(api.vkCreateSampler(device, &sampler, nullptr, &result->sampler), "vkCreateSampler");
        std::vector<std::byte> pixels;
        std::vector<VkBufferImageCopy> copies;
        const std::size_t alignment = std::max(4u, config.components * config.componentBytes);
        for (const auto& upload : uploads) {
            pixels.resize((pixels.size() + alignment - 1) / alignment * alignment);
            VkBufferImageCopy copy{}; copy.bufferOffset = pixels.size();
            copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, upload.level, 0, 1};
            copy.imageExtent = {upload.width, upload.height, 1}; copies.push_back(copy);
            pixels.insert(pixels.end(), upload.pixels.begin(), upload.pixels.end());
        }
        const auto staging = pixels.empty() ? BufferRef{} : sharedBuffer(pixels.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pixels.data());
        check(api.vkResetCommandBuffer(textureCommandBuffer, 0), "vkResetCommandBuffer(texture)");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(api.vkBeginCommandBuffer(textureCommandBuffer, &begin), "vkBeginCommandBuffer(texture)");
        const auto barrier = [&](std::uint32_t first, std::uint32_t levels, VkImageLayout oldLayout,
                                 VkImageLayout newLayout, VkAccessFlags source, VkAccessFlags destination,
                                 VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage,
                                 VkImage target = VK_NULL_HANDLE) {
            VkImageMemoryBarrier transition{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            transition.srcAccessMask = source; transition.dstAccessMask = destination;
            transition.oldLayout = oldLayout; transition.newLayout = newLayout;
            transition.srcQueueFamilyIndex = transition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            transition.image = target ? target : image.image;
            transition.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, first, levels, 0, 1};
            api.vkCmdPipelineBarrier(textureCommandBuffer, sourceStage, destinationStage, 0, 0, nullptr,
                                     0, nullptr, 1, &transition);
        };
        barrier(0, config.levels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        if (previous) {
            barrier(0, config.levels, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT), VK_PIPELINE_STAGE_TRANSFER_BIT, previous->image.image);
            std::vector<VkImageCopy> regions;
            for (std::uint32_t level = 0; level < config.levels; ++level) {
                VkImageCopy copy{}; copy.srcSubresource = copy.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
                copy.extent = {std::max(1u, config.width >> level), std::max(1u, config.height >> level), 1};
                regions.push_back(copy);
            }
            api.vkCmdCopyImage(textureCommandBuffer, previous->image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::uint32_t(regions.size()), regions.data());
            barrier(0, config.levels, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT), previous->image.image);
        } else {
            const VkClearColorValue zero{};
            const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, config.levels, 0, 1};
            api.vkCmdClearColorImage(textureCommandBuffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &zero, 1, &range);
        }
        barrier(0, config.levels, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        if (staging) api.vkCmdCopyBufferToImage(textureCommandBuffer, staging->buffer, image.image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, std::uint32_t(copies.size()), copies.data());
        std::uint32_t previousWidth = config.width, previousHeight = config.height;
        for (std::uint32_t level = 1; config.generateMipmaps && level < config.levels; ++level) {
            barrier(level - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            const auto nextWidth = std::max(1u, previousWidth / 2), nextHeight = std::max(1u, previousHeight / 2);
            VkImageBlit blit{}; blit.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1};
            blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1};
            blit.srcOffsets[1] = {int(previousWidth), int(previousHeight), 1};
            blit.dstOffsets[1] = {int(nextWidth), int(nextHeight), 1};
            api.vkCmdBlitImage(textureCommandBuffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
            barrier(level - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));
            previousWidth = nextWidth; previousHeight = nextHeight;
        }
        barrier(config.generateMipmaps ? config.levels - 1 : 0, config.generateMipmaps ? 1 : config.levels,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));
        check(api.vkEndCommandBuffer(textureCommandBuffer), "vkEndCommandBuffer(texture)");
        check(api.vkResetFences(device, 1, &textureFence), "vkResetFences(texture)");
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1; submit.pCommandBuffers = &textureCommandBuffer;
        check(api.vkQueueSubmit(queue, 1, &submit, textureFence), "vkQueueSubmit(texture)");
        check(api.vkWaitForFences(device, 1, &textureFence, VK_TRUE, UINT64_MAX), "vkWaitForFences(texture)");
        return result;
    }
    TextureRef fallbackTexture(bool white) {
        auto& texture = white ? whiteTexture : emptyTexture;
        if (!texture) texture = createTextureResource(TextureConfig{}, {{0, 1, 1,
            std::vector<std::byte>(4, white ? std::byte{255} : std::byte{0})}}, {}, false);
        return texture;
    }
    TextureRef updateTexture(const std::shared_ptr<Texture>& texture, std::shared_ptr<TextureEntry>& binding) {
        if (!texture) return fallbackTexture(true);
        auto found = textureCache.find(texture->id);
        auto cached = found == textureCache.end() ? nullptr : found->second;
        if (texture->version > 0 && (!cached || cached->version != texture->version)) {
            const auto source = texture->source;
            const auto image = source ? source->data : nullptr;
            if (!image) {
                binding = cached ? cached->entry : nullptr;
                return binding && binding->resource ? binding->resource : fallbackTexture(false);
            }
            auto config = textureConfig(*texture, *image);
            const auto key = textureKey(*texture);
            if (!cached) {
                cached = std::make_shared<TextureCache>(); cached->owner = texture;
                cached->disposeListener = texture->addEventListener("dispose", [this, id = texture->id](auto&) { removeTexture(id); });
                textureCache[texture->id] = cached;
            }
            auto& sourceSlot = sourceCache[source->id];
            if (!sourceSlot) { sourceSlot = std::make_shared<SourceCache>(); sourceSlot->owner = source; }
            const auto sharedSource = sourceSlot;
            auto entry = sharedSource->entries[key].lock();
            const bool forceUpload = !entry || !entry->resource;
            if (!entry) { entry = std::make_shared<TextureEntry>(); sharedSource->entries[key] = entry; }
            if (cached->entry != entry || cached->entryRetired) {
                releaseTextureEntry(cached);
                cached->source = sharedSource; cached->entry = entry; cached->key = key;
                cached->entryRetired = false;
                ++entry->usedTimes;
            }
            if (forceUpload || sharedSource->version != source->version) {
                const auto mipmaps = texture->mipmaps;
                const bool manual = mipmaps && !mipmaps->empty();
                if (entry->resource) {
                    const auto& previous = entry->resource->config;
                    if ((!manual && (previous.width != config.width || previous.height != config.height)) ||
                        config.width > previous.width || config.height > previous.height)
                        throw std::invalid_argument("Resizing an uploaded texture requires dispose before needsUpdate");
                    // texStorage is immutable; removing mipmap descriptors does not discard allocated levels.
                    config.width = previous.width; config.height = previous.height; config.levels = previous.levels;
                }
                if (config.width == 0 || config.height == 0)
                    throw std::invalid_argument("Initial texture storage dimensions must be positive");
                const auto maximumLevels = std::uint32_t(std::floor(std::log2(std::max(config.width, config.height)))) + 1;
                if (config.levels > maximumLevels || (manual && mipmaps->size() > config.levels))
                    throw std::invalid_argument("DataTexture mipmap count exceeds its allocated image levels");
                auto pixels = entry->pixels;
                const std::size_t pixelBytes = config.components * config.componentBytes;
                if (pixels.empty()) pixels.resize(std::size_t(config.width) * config.height * pixelBytes);
                std::vector<TextureUpload> uploads;
                if (source->dataReady && manual) {
                    for (std::size_t level = 0; level < mipmaps->size(); ++level) {
                        const auto mipmap = mipmaps->at(level);
                        const auto width = std::max(1u, config.width >> level), height = std::max(1u, config.height >> level);
                        if (!mipmap || !std::isfinite(mipmap->width) || !std::isfinite(mipmap->height) ||
                            mipmap->width < 0 || mipmap->height < 0 || std::trunc(mipmap->width) != mipmap->width ||
                            std::trunc(mipmap->height) != mipmap->height || mipmap->width > width || mipmap->height > height)
                            throw std::invalid_argument("DataTexture mipmap dimensions exceed the allocated level");
                        auto levelConfig = config;
                        levelConfig.width = std::uint32_t(mipmap->width); levelConfig.height = std::uint32_t(mipmap->height);
                        auto incoming = texturePixels(*mipmap, levelConfig);
                        if (incoming.empty()) continue;
                        if (level == 0) for (std::uint32_t row = 0; row < levelConfig.height; ++row)
                            std::memcpy(pixels.data() + std::size_t(row) * config.width * pixelBytes,
                                        incoming.data() + std::size_t(row) * levelConfig.width * pixelBytes,
                                        std::size_t(levelConfig.width) * pixelBytes);
                        uploads.push_back({std::uint32_t(level), levelConfig.width, levelConfig.height, std::move(incoming)});
                    }
                } else if (source->dataReady) {
                    const auto ranges = texture->updateRanges;
                    const bool partial = ranges && !ranges->empty();
                    if (partial && config.components != 4)
                        throw std::invalid_argument("Texture updateRanges require RGBA components");
                    auto sourceConfig = config;
                    // Each upstream partial upload has height one, so flipY does not move its destination row.
                    if (partial) sourceConfig.flipY = false;
                    const auto incoming = texturePixels(*image, sourceConfig);
                    if (!partial) pixels = incoming;
                    else {
                        for (const auto& range : *ranges)
                            if (!range || !std::isfinite(range->start) || !std::isfinite(range->count) ||
                                range->start < 0 || range->count < 0)
                                throw std::invalid_argument("Invalid texture update range");
                        v8ArraySort(*ranges, [](const auto& a, const auto& b) {
                            const auto left = a, right = b;
                            return left->start - right->start;
                        });
                        std::size_t mergeIndex = 0;
                        const auto row = [&](double value) { return std::floor(value / (config.width * 4.0)); };
                        for (std::size_t i = 1; i < ranges->size(); ++i) {
                            auto& previous = (*ranges)[mergeIndex]; const auto current = (*ranges)[i];
                            if (current->start <= previous->start + previous->count + 1 && row(current->start) == row(previous->start) &&
                                row(current->start + current->count - 1) == row(current->start))
                                previous->count = std::max(previous->count, current->start + current->count - previous->start);
                            else (*ranges)[++mergeIndex] = current;
                        }
                        ranges->resize(mergeIndex + 1);
                        for (const auto& range : *ranges) {
                            const double pixelStart = std::floor(range->start / 4), pixelCount = std::ceil(range->count / 4);
                            if (pixelStart >= double(config.width) * config.height || pixelCount > config.width - std::fmod(pixelStart, config.width))
                                throw std::invalid_argument("Texture update range exceeds a single image row");
                            const auto offset = std::size_t(pixelStart) * pixelBytes;
                            std::memcpy(pixels.data() + offset, incoming.data() + offset, std::size_t(pixelCount) * pixelBytes);
                        }
                    }
                    uploads.push_back({0, config.width, config.height, pixels});
                }
                if (source->dataReady || !entry->resource) {
                    entry->resource = createTextureResource(config, uploads, entry->resource);
                    if (!entry->memoryTracked) {
                        entry->memoryTracked = true;
                        adjustMemory("textures", 1);
                    }
                    entry->pixels = std::move(pixels);
                    if (source->dataReady) { entry->dataInitialized = true; ++info.textureUploads; }
                }
                if (manual) texture->generateMipmaps = false;
                else if (source->dataReady && texture->updateRanges && !texture->updateRanges->empty()) texture->clearUpdateRanges();
                sharedSource->version = source->version;
                const auto callback = texture->onUpdate;
                if (callback) callback(*texture);
            }
            // WebGLTextures records the texture version after its upload callback.
            cached->version = texture->version;
        }
        binding = cached ? cached->entry : nullptr;
        return binding && binding->resource ? binding->resource : fallbackTexture(false);
    }
    void registerClippingMaterial(const std::shared_ptr<Material>& material, const JSValue& properties) {
        const auto found = clippingMaterials.find(material->id);
        const auto previous = found == clippingMaterials.end() ? nullptr : found->second;
        if (previous)
            if (const auto state = previous->properties.lock(); state && state->sameValue(properties)) return;
        auto cached = std::make_shared<ClippingMaterial>(); cached->owner = material;
        // getProgram obtains the current properties again for a new material state.
        cached->properties = JSWeakValue(clippingProperties->get(nativeMaterial(material)));
        if (previous) cached->listener = previous->listener;
        else cached->listener = std::make_shared<std::function<void(Event&)>>([token = lifecycle, id = material->id](auto&) {
            const auto renderer = token->owner;
            if (!renderer) return;
            const auto found = renderer->clippingMaterials.find(id);
            if (found == renderer->clippingMaterials.end()) return;
            const auto retired = found->second;
            if (const auto owner = retired->owner.lock()) {
                owner->removeEventListener("dispose", retired->listener);
                const auto propertyObject = renderer->clippingProperties->object();
                propertyObject.get("remove").call(propertyObject, {nativeMaterial(owner)});
            }
            if (token->owner != renderer) return;
            const auto current = renderer->clippingMaterials.find(id);
            if (current != renderer->clippingMaterials.end() && current->second == retired)
                renderer->clippingMaterials.erase(current);
        });
        material->addEventListener("dispose", cached->listener);
        clippingMaterials[material->id] = std::move(cached);
    }
    VkDescriptorSet textureDescriptor(const TextureRef& map, const TextureRef& alpha, const BufferRef& uniforms,
                                      const BufferRef& matrices, const BufferRef& colors,
                                      const BufferRef& morph, const TextureRef& weights,
                                      const BufferRef& positions, const BufferRef& vertexColors,
                                      const BufferRef& mapUv, const BufferRef& alphaUv, const BufferRef& clipPlanes) {
        VkDescriptorSet set = VK_NULL_HANDLE;
        for (;;) {
            if (activeDescriptorPool == descriptorPools.size()) {
                VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12288},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32768}};
                VkDescriptorPoolCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
                create.maxSets = 4096; create.poolSizeCount = 3; create.pPoolSizes = sizes;
                VkDescriptorPool pool;
                check(api.vkCreateDescriptorPool(device, &create, nullptr, &pool), "vkCreateDescriptorPool");
                descriptorPools.push_back(pool);
            }
            VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocate.descriptorPool = descriptorPools[activeDescriptorPool];
            allocate.descriptorSetCount = 1; allocate.pSetLayouts = &textureLayout;
            const auto result = api.vkAllocateDescriptorSets(device, &allocate, &set);
            if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) { ++activeDescriptorPool; continue; }
            check(result, "vkAllocateDescriptorSets"); break;
        }
        VkDescriptorImageInfo images[] = {{map->sampler, map->image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                           {alpha->sampler, alpha->image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                           {weights->sampler, weights->image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
        VkDescriptorBufferInfo buffers[] = {{uniforms->buffer, 0, uniforms->size},
            {matrices->buffer, 0, matrices->size}, {colors->buffer, 0, colors->size}, {morph->buffer, 0, morph->size},
            {positions->buffer, 0, positions->size}, {vertexColors->buffer, 0, vertexColors->size},
            {mapUv->buffer, 0, mapUv->size}, {alphaUv->buffer, 0, alphaUv->size},
            {clipPlanes->buffer, 0, clipPlanes->size}};
        VkWriteDescriptorSet writes[12]{};
        for (std::uint32_t i = 0; i < 12; ++i) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; writes[i].dstSet = set;
            writes[i].dstBinding = i; writes[i].descriptorCount = 1;
            writes[i].descriptorType = i < 2 || i == 6 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                : i == 2 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            if (i < 2 || i == 6) writes[i].pImageInfo = &images[i == 6 ? 2 : i];
            else writes[i].pBufferInfo = &buffers[i < 6 ? i - 2 : i - 3];
        }
        api.vkUpdateDescriptorSets(device, 12, writes, 0, nullptr);
        return set;
    }
    static BufferAttribute::Array uploadedArray(const std::vector<std::byte>& bytes, double type) {
        std::vector<std::uint8_t> values(bytes.size());
        if (!bytes.empty()) std::memcpy(values.data(), bytes.data(), bytes.size());
        Uint8Array storage(values);
        switch (int(type)) {
        case 5120: return Int8Array(storage.buffer, 0, double(bytes.size()));
        case 5121: return storage;
        case 5122: return Int16Array(storage.buffer, 0, double(bytes.size() / 2));
        case 5123: case 5131: return Uint16Array(storage.buffer, 0, double(bytes.size() / 2));
        case 5124: return Int32Array(storage.buffer, 0, double(bytes.size() / 4));
        case 5125: return Uint32Array(storage.buffer, 0, double(bytes.size() / 4));
        case 5126: return Float32Array(storage.buffer, 0, double(bytes.size() / 4));
        default: throw std::invalid_argument("Unsupported uploaded vertex attribute format");
        }
    }
    void syncAttribute(const std::shared_ptr<AttributeCache>& cached, const BufferAttribute& attribute,
                       const JSValue& capturedBuffer = {}) {
        // The upload data belongs to WebGLAttributes' ephemeron. A derived draw
        // cache must not keep that data (and its raw Vulkan allocation) alive
        // after the managed attribute key becomes unreachable.
        const auto data = cached->data.lock();
        if (!data || data->isUndefined())
            throw std::runtime_error("Attribute upload cache was retired before binding");
        const auto buffer = capturedBuffer.isUndefined() ? data->get("buffer") : capturedBuffer;
        const auto previousBuffer=cached->boundBuffer.lock();
        if(!previousBuffer || !previousBuffer->sameValue(buffer)) {
            cached->boundBuffer=JSWeakValue(buffer);cached->revision=UINT64_MAX;
        }
        const auto revision = attributeCommands->revision(buffer);
        if (cached->snapshot && revision == cached->revision) return;
        // The upper cache owns version/range/callback decisions. The adapter's
        // revision observes completed byte writes, including writes preceding a
        // later JavaScript exception. Draw conversions never re-upload an array.
        const auto type = data->get("type").toNumber();
        const auto bytes = attributeCommands->read(buffer);
        auto snapshot = std::make_shared<AttributeSnapshot>(uploadedArray(bytes, type), attribute,
                                                            type == 5131);
        const std::vector<ComponentRange> ranges{{0, componentCount(*snapshot)}};
        for (std::size_t i = 0; i < cached->vertices.size(); ++i)
            if (cached->vertices[i])
                updateBuffer(cached->vertices[i], vertexValues(*snapshot, i != 0), ranges,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        if (cached->indices)
            updateBuffer(cached->indices, indexValues(*snapshot), ranges, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        cached->snapshot = std::move(snapshot);
        cached->revision = revision;
    }
        static bool samePointer(const VulkanBindingCommands::Attribute& a,
                            const VulkanBindingCommands::Attribute& b) {
        return a.enabled == b.enabled && a.normalized == b.normalized && a.integer == b.integer &&
            a.size == b.size && a.type == b.type && a.stride == b.stride && a.offset == b.offset &&
            a.divisor == b.divisor && a.value == b.value;
    }
    std::shared_ptr<AttributeCache> boundInput(const VulkanBindingCommands::Attribute& pointer,
                                             unsigned outputSize) {
        // MeshBasicMaterial has floating shader inputs. Integer pointers require
        // matching integer shader declarations, not a numeric conversion here.
        if(pointer.enabled && pointer.integer)
            throw std::invalid_argument("Integer vertex attributes cannot bind a floating-point shader input");
        BoundInput* previous = nullptr;
        for(auto& input : boundInputs) {
            if(input.outputSize != outputSize || !samePointer(input.pointer,pointer))continue;
            const auto key=input.buffer.lock();
            if((!pointer.enabled && pointer.buffer.isNull()) || (key && key->sameValue(pointer.buffer))) {
                previous=&input;break;
            }
        }
        if(!previous) {
            BoundInput input;
            input.buffer=JSWeakValue(pointer.buffer);input.pointer=pointer;input.pointer.buffer={};
            input.outputSize=outputSize;input.cached=std::make_shared<AttributeCache>();
            boundInputs.push_back(std::move(input));previous=&boundInputs.back();
        }
        const auto cached=previous->cached;
        const auto revision=pointer.enabled ? attributeCommands->revision(pointer.buffer) : 0;
        if(cached->snapshot && cached->revision==revision)return cached;
        auto actual=pointer;
        BufferAttribute::Array values;
        if(pointer.enabled)values=uploadedArray(attributeCommands->read(pointer.buffer),pointer.type);
        else {
            values=Float32Array(std::vector<float>(pointer.value.begin(),pointer.value.end()));
            actual.size=4;actual.type=0x1406;actual.normalized=false;actual.stride=0;actual.offset=0;
        }
        auto source=std::make_shared<AttributeSnapshot>(std::move(values),actual,outputSize);
        const auto normalized=source->normalized;
        const auto flattened=vertexValues(*source,normalized);
        if(!pointer.enabled) {source->constantInput=true;source->count=std::numeric_limits<double>::infinity();}
        // Keep the fetch view for bounds and CPU fog/morph; the GPU conversion
        // is packed, independent of the source's byte stride and offset.
        if(cached->vertices[false])updateBuffer(cached->vertices[false],flattened,{{0,flattened.size()}},
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        else cached->vertices[false]=sharedBuffer(std::max<std::size_t>(flattened.size(),1)*sizeof(float),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            flattened.empty()?nullptr:flattened.data(),true);
        cached->snapshot=std::move(source);cached->revision=revision;return cached;
    }
    std::shared_ptr<AttributeCache> boundMatrix(const VulkanBindingCommands::Snapshot& bindings) {
        std::array<VulkanBindingCommands::Attribute,4> pointers;
        for(unsigned i=0;i<4;++i)pointers[i]=bindings.attributes[5+i];
        MatrixInput* previous=nullptr;
        for(auto& input:matrixInputs) {
            bool same=true;
            for(unsigned i=0;i<4 && same;++i) {
                same=samePointer(input.pointers[i],pointers[i]);
                if(pointers[i].enabled){const auto key=input.buffers[i].lock();same=same && key && key->sameValue(pointers[i].buffer);}
            }
            if(same){previous=&input;break;}
        }
        if(!previous) {
            MatrixInput input;
            for(unsigned i=0;i<4;++i){input.buffers[i]=JSWeakValue(pointers[i].buffer);input.pointers[i]=pointers[i];input.pointers[i].buffer={};}
            input.revisions.fill(UINT64_MAX);input.cached=std::make_shared<AttributeCache>();
            matrixInputs.push_back(std::move(input));previous=&matrixInputs.back();
        }
        std::array<std::uint64_t,4> revisions{};
        std::array<std::shared_ptr<AttributeSnapshot>,4> columns;
        bool changed=false;std::size_t rows=SIZE_MAX;
        for(unsigned i=0;i<4;++i) {
            revisions[i]=pointers[i].enabled?attributeCommands->revision(pointers[i].buffer):0;
            changed=changed || revisions[i]!=previous->revisions[i];
            if(pointers[i].enabled) {
                if(pointers[i].integer)throw std::invalid_argument("Integer instance attributes cannot bind a floating-point shader input");
                columns[i]=std::make_shared<AttributeSnapshot>(uploadedArray(attributeCommands->read(pointers[i].buffer),pointers[i].type),pointers[i],4);
                rows=std::min(rows,std::size_t(columns[i]->count));
            } else {
                auto constant=pointers[i];constant.size=4;constant.type=0x1406;constant.normalized=false;constant.offset=constant.stride=0;
                columns[i]=std::make_shared<AttributeSnapshot>(Float32Array(std::vector<float>(constant.value.begin(),constant.value.end())),constant,4);
            }
        }
        if(rows==SIZE_MAX)rows=1;
        auto cached=previous->cached;
        if(cached->snapshot && !changed)return cached;
        std::vector<float> values(rows*16);
        for(std::size_t row=0;row<rows;++row)for(unsigned col=0;col<4;++col)for(unsigned c=0;c<4;++c)
            values[row*16+col*4+c]=float(columns[col]->getComponent(pointers[col].enabled?double(row):0,c));
        VulkanBindingCommands::Attribute packed;packed.size=16;packed.type=0x1406;
        auto snapshot=std::make_shared<AttributeSnapshot>(Float32Array(values),packed,16);
        if(cached->vertices[false])updateBuffer(cached->vertices[false],values,{{0,values.size()}},VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        else cached->vertices[false]=sharedBuffer(std::max<std::size_t>(values.size(),1)*sizeof(float),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,values.empty()?nullptr:values.data(),true);
        cached->snapshot=std::move(snapshot);previous->revisions=revisions;return cached;
    }
    JSValue bindingProgram(const MeshBasicMaterial& material,const std::shared_ptr<BufferGeometry>& geometry,
                           const InstancedMesh* instanced,const std::shared_ptr<Texture>& map,
                           const std::shared_ptr<Texture>& alpha,bool colors,
                           std::uint32_t numPlanes, std::uint32_t numIntersection) {
        const auto color=geometry->getAttribute("color");
        const std::string key=std::to_string(material.vertexColors)+":"+std::to_string(colors)+":"+
            std::to_string(colors && color && color->getItemSize()==4)+":"+std::to_string(bool(instanced))+":"+
            std::to_string(instanced && bool(instanced->instanceColor))+":"+
            (map?std::to_string(map->channel):"-")+":"+(alpha?std::to_string(alpha->channel):"-")+":"+
            std::to_string(double(material.alphaTest)>0)+":"+std::to_string(material.premultipliedAlpha)+":"+
            std::to_string(material.side)+":"+std::to_string(material.fog)+":"+
            std::to_string(geometry->morphAttributes.contains("position"))+":"+
            std::to_string(instanced && bool(instanced->morphTexture))+":"+
            std::to_string(numPlanes)+":"+std::to_string(numIntersection);
        if(const auto found=programs.find(key);found!=programs.end())return found->second;
        const auto literal=[](JSValue target,const char* name,JSValue value) {
            JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;
            target.defineProperty(name,d);
        };
        auto inputs=JSValue::object();
        const auto add=[&](const std::string& name,int location,int size) {
            auto value=JSValue::object();literal(value,"location",location);literal(value,"locationSize",size);
            literal(inputs,name.c_str(),value);
        };
        add("position",0,1);if(colors)add("color",1,1);
        const auto uvName=[](double channel){return channel==0?std::string("uv"):"uv"+std::to_string(int(channel));};
        if(map)add(uvName(map->channel),3,1);
        if(alpha && (!map || alpha->channel!=map->channel))add(uvName(alpha->channel),4,1);
        if(instanced){add("instanceMatrix",5,4);if(instanced->instanceColor)add("instanceColor",9,1);}
        auto program=JSValue::object();literal(program,"id",double(programs.size()));
        literal(program,"getAttributes",JSValue::function(+[](const JSValue&,const auto&,const auto& captures){return captures[0];},{inputs}));
        programs.emplace(key,program);return program;
    }

    void trackAttribute(const JSValue& key) {
        // Tracking the native extension's dispose event requires no managed field
        // reads. Raw uploads also cover unused attributes without decoding them.
        if (!key.isHostObject("three.BufferAttribute")) return;
        const auto attribute = std::dynamic_pointer_cast<BufferAttribute>(::vertexAttribute(key));
        auto found = attributeCache.find(attribute->id);
        if (found != attributeCache.end() && found->second->owner.lock() != attribute) {
            // A native handle may have expired while its managed attribute stayed
            // alive. Preserve the authoritative managed cache in that case.
            attributeCache.erase(found);
            found = attributeCache.end();
        }
        if (found != attributeCache.end()) return;
        auto cached = std::make_shared<AttributeCache>();
        cached->owner = attribute;
        cached->key = JSWeakValue(key);
        cached->disposeListener = attribute->addEventListener("dispose",
            [weak = std::weak_ptr<Lifecycle>(lifecycle), id = attribute->id](auto&) {
                if (const auto lifetime = weak.lock(); lifetime && lifetime->owner)
                    lifetime->owner->removeAttribute(id);
            });
        attributeCache[attribute->id] = std::move(cached);
    }
    std::shared_ptr<AttributeCache> decodeAttribute(const std::shared_ptr<BufferAttribute>& attribute,
                                                   bool index = false,const JSValue& capturedBuffer = {}) {
        const auto key = nativeAttribute(attribute);
        const auto data = attributes->get(key);
        if (data.isUndefined())
            throw std::runtime_error("Attribute upload callback removed its buffer before binding");
        trackAttribute(key);
        const auto cached = attributeCache.at(attribute->id);
        const auto previousData = cached->data.lock();
        if (!previousData || !previousData->sameValue(data)) {
            cached->data = JSWeakValue(data);
            cached->revision = std::numeric_limits<std::uint64_t>::max();
        }
        syncAttribute(cached, *attribute,capturedBuffer);
        if (index) {
            if (!cached->indices) {
                const auto values = indexValues(*cached->snapshot);
                cached->indices = sharedBuffer(std::max<std::size_t>(values.size(), 1) * sizeof(values[0]),
                                              VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                              values.empty() ? nullptr : values.data(), true);
            }
        } else if (!cached->vertices[attribute->normalized]) {
            const auto values = vertexValues(*cached->snapshot, attribute->normalized);
            cached->vertices[attribute->normalized] =
                sharedBuffer(std::max<std::size_t>(values.size(), 1) * sizeof(values[0]),
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                             values.empty() ? nullptr : values.data(), true);
        }
        return cached;
    }
    std::shared_ptr<AttributeCache> uploadIndex(const std::shared_ptr<BufferAttribute>& attribute) {
        // WebGLBindingStates.setup uploads its effective index only after a valid
        // draw range. Vertex/instance uploads already happened in WebGLObjects.
        managedAttributes.get("update").call(managedAttributes,
            {nativeAttribute(attribute), attributeCommands->object().get("ELEMENT_ARRAY_BUFFER")});
        return decodeAttribute(attribute, true);
    }
    std::shared_ptr<AttributeCache> vertexAttribute(const std::shared_ptr<BufferAttribute>& attribute) {
        const auto found = attributeCache.find(attribute->id);
        if (found == attributeCache.end() || found->second->owner.lock() != attribute)
            throw std::invalid_argument("Introducing uncached vertex attributes inside render callbacks is not supported");
        auto cached = decodeAttribute(attribute);
        cached->snapshot->itemSize = attribute->itemSize;
        cached->snapshot->count = attribute->count;
        cached->snapshot->normalized = attribute->normalized;
        if (!std::isfinite(attribute->itemSize) || attribute->itemSize <= 0 ||
            std::trunc(attribute->itemSize) != attribute->itemSize)
            throw std::invalid_argument("Vulkan vertex attribute itemSize must be a positive integer");
        if (!cached->vertices[attribute->normalized]) {
            const auto values = vertexValues(*cached->snapshot, attribute->normalized);
            cached->vertices[attribute->normalized] =
                sharedBuffer(std::max<std::size_t>(values.size(), 1) * sizeof(values[0]),
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                             values.empty() ? nullptr : values.data(), true);
        }
        return cached;
    }
    std::shared_ptr<GeometryCache> registerGeometry(const std::shared_ptr<BufferGeometry>& geometry) {
        const auto found = geometryCache.find(geometry->id);
        if (found != geometryCache.end() && found->second->owner.lock() == geometry)
            return found->second;
        auto cached = std::make_shared<GeometryCache>();
        cached->owner = geometry;
        geometryCache[geometry->id] = cached;
        return cached;
    }
    // The size is explicit so a second target can be created without disturbing the
    // renderer's current one; zero means "use the active size", as every call site does.
    void createImage(Image& result, VkFormat format, VkImageUsageFlags usage,
                     VkImageAspectFlags aspect, std::uint32_t layers = 1,
                     std::uint32_t imageWidth = 0, std::uint32_t imageHeight = 0) {
        if (imageWidth == 0) imageWidth = width;
        if (imageHeight == 0) imageHeight = height;
        VkImageCreateInfo create{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        create.imageType = VK_IMAGE_TYPE_2D;
        create.format = format;
        create.extent = {imageWidth, imageHeight, 1};
        create.mipLevels = 1;
        create.arrayLayers = layers;
        create.samples = VK_SAMPLE_COUNT_1_BIT;
        create.tiling = VK_IMAGE_TILING_OPTIMAL;
        create.usage = usage;
        create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (layers > 1) create.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
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
        view.viewType = layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        view.format = format;
        view.subresourceRange.aspectMask = aspect;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = layers;
        check(api.vkCreateImageView(device, &view, nullptr, &result.view), "vkCreateImageView");
        if (layers == 6) {
            VkImageViewCreateInfo cube = view;
            cube.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            check(api.vkCreateImageView(device, &cube, nullptr, &result.cubeView),
                  "vkCreateImageView(cube)");
        }
    }
    void destroyImage(Image& image) {
        if (image.view)
            api.vkDestroyImageView(device, image.view, nullptr);
        if (image.cubeView) api.vkDestroyImageView(device, image.cubeView, nullptr);
        if (image.image)
            api.vkDestroyImage(device, image.image, nullptr);
        if (image.memory)
            api.vkFreeMemory(device, image.memory, nullptr);
        image = {};
    }
    void destroyTargets() {

        // The per target teardown releases its own framebuffer.

        if (active) { destroyGpuTarget(*active); active.reset(); }


        destroyBuffer(readback);
    }
    // One 2D view and framebuffer per cube face, rebuilt when the active face changes.
    void selectFace(std::uint32_t face) {
        if (active->layers <= 1) return;
        if (face >= active->layers) throw std::out_of_range("active cube face out of range");
        activeCubeFace = face;
        if (active->faceFramebuffer) api.vkDestroyFramebuffer(device, active->faceFramebuffer, nullptr);
        active->faceFramebuffer = VK_NULL_HANDLE;
        if (active->faceView) api.vkDestroyImageView(device, active->faceView, nullptr);
        active->faceView = VK_NULL_HANDLE;
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = active->color.image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = VK_FORMAT_R8G8B8A8_UNORM;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.baseArrayLayer = face;
        view.subresourceRange.layerCount = 1;
        check(api.vkCreateImageView(device, &view, nullptr, &active->faceView), "vkCreateImageView(face)");
        const VkImageView attachments[] = {active->faceView, active->depth.view};
        VkFramebufferCreateInfo create{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        create.renderPass = renderPass;
        create.attachmentCount = 2;
        create.pAttachments = attachments;
        create.width = width;
        create.height = height;
        create.layers = 1;
        check(api.vkCreateFramebuffer(device, &create, nullptr, &active->faceFramebuffer),
              "vkCreateFramebuffer(face)");
    }
    // Builds a complete GPU target without touching the renderer current one.
    std::unique_ptr<GpuTarget> createGpuTarget(std::uint32_t targetWidth,
                                              std::uint32_t targetHeight,
                                              std::uint32_t targetLayerCount) {
        auto target = std::make_unique<GpuTarget>();
        target->width = targetWidth;
        target->height = targetHeight;
        target->layers = targetLayerCount;
        createImage(target->color, VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, targetLayerCount, targetWidth, targetHeight);
        createImage(target->depth, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT, 1, targetWidth, targetHeight);
        const VkImageView views[] = {target->color.view, target->depth.view};
        VkFramebufferCreateInfo create{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        create.renderPass = renderPass;
        create.attachmentCount = 2;
        create.pAttachments = views;
        create.width = targetWidth;
        create.height = targetHeight;
        create.layers = 1;
        check(api.vkCreateFramebuffer(device, &create, nullptr, &target->framebuffer),
              "vkCreateFramebuffer(gpu target)");
        return target;
    }
    void destroyGpuTarget(GpuTarget& target) {
        if (target.framebuffer) api.vkDestroyFramebuffer(device, target.framebuffer, nullptr);
        destroyImage(target.color);
        destroyImage(target.depth);
    }

    void resize(std::uint32_t newWidth, std::uint32_t newHeight, std::uint32_t layers = 1) {
        if (!newWidth || !newHeight)
            throw std::invalid_argument("Renderer dimensions must be positive");
        VkPhysicalDeviceProperties properties{};
        api.vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        if (newWidth > properties.limits.maxFramebufferWidth ||
            newHeight > properties.limits.maxFramebufferHeight)
            throw std::invalid_argument("Renderer dimensions exceed Vulkan device limits");
        if (active && newWidth == active->width && newHeight == active->height &&
            layers == active->layers)
            return;
        check(api.vkDeviceWaitIdle(device), "vkDeviceWaitIdle");
        destroyTargets();
        width = newWidth;
        height = newHeight;
        active = createGpuTarget(newWidth, newHeight, layers);
        selectFace(activeCubeFace);
        readback = createBuffer(static_cast<VkDeviceSize>(width) * height * 4,
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT);
        active->pixels.assign(static_cast<std::size_t>(width) * height * 4, 0);
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

    VkPipeline pipeline(const MeshBasicMaterial& material, int side, bool mirrored, bool reverse,
                        std::uint32_t positionStride, std::uint32_t colorSize, bool colors,
                        bool fog, std::uint32_t mapSize, std::uint32_t alphaSize, bool mapUv, bool alphaUv) {
        if (side < 0 || side > 2 || material.blending < 0 || material.blending > 4 ||
            material.depthFunc < 0 || material.depthFunc > 7)
            throw std::invalid_argument(
                "Unsupported material side, blending mode or depth function");
        const auto colorStride = colorSize * sizeof(float);
        if (positionStride > maxVertexStride || colorStride > maxVertexStride ||
            positionStride > UINT16_MAX || colorStride > UINT16_MAX ||
            mapSize > maxVertexStride / sizeof(float) || alphaSize > maxVertexStride / sizeof(float))
            throw std::invalid_argument("Attribute itemSize exceeds Vulkan vertex binding limits");
        const std::uint64_t baseKey =
            static_cast<std::uint64_t>(side) | (std::uint64_t(mirrored) << 2) |
            (std::uint64_t(material.depthTest) << 3) | (std::uint64_t(material.depthWrite) << 4) |
            (std::uint64_t(material.colorWrite) << 5) | (std::uint64_t(material.transparent) << 6) |
            (std::uint64_t(material.blending) << 7) |
            (std::uint64_t(material.premultipliedAlpha) << 10) |
            (std::uint64_t(material.depthFunc) << 11) | (std::uint64_t(material.wireframe) << 15) |
            (std::uint64_t(reverse) << 16) | (std::uint64_t(positionStride) << 17) |
            (std::uint64_t(colorStride) << 33) | (std::uint64_t(colors) << 49) |
            (std::uint64_t(fog) << 50) | (std::uint64_t(colorSize == 4) << 51);
        const std::string key = std::to_string(baseKey) + ":" + std::to_string(mapSize) + ":" +
            std::to_string(alphaSize) + ":" + std::to_string(mapUv) + ":" + std::to_string(alphaUv);
        if (auto found = pipelines.find(key); found != pipelines.end())
            return found->second;
        const auto stages = shaderModules->pipelineStages();
        // Vertex pulling consumes the binding module's explicit byte-layout
        // snapshot. It supports all integer divisors without device extensions.
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
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
        create.pStages = stages.data();
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

    // Upstream draws a BoxGeometry(1, 1, 1) through the PMREM pass. Its twelve triangles
    // are a constant, so they ship as one vertex buffer built at construction.
    void createPmremGeometry() {
        const std::array<float, 108> boxPositions{
            1, -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, -1, 1, 1, 1, 1, -1, 1,
            -1, -1, 1, -1, 1, 1, -1, 1, -1, -1, -1, 1, -1, 1, -1, -1, -1, -1,
            -1, 1, -1, -1, 1, 1, 1, 1, 1, -1, 1, -1, 1, 1, 1, 1, 1, -1,
            -1, -1, 1, -1, -1, -1, 1, -1, -1, -1, -1, 1, 1, -1, -1, 1, -1, 1,
            -1, -1, 1, 1, -1, 1, 1, 1, 1, -1, -1, 1, 1, 1, 1, -1, 1, 1,
            1, -1, -1, -1, -1, -1, -1, 1, -1, 1, -1, -1, -1, 1, -1, 1, 1, -1};
        pmremVertices = createBuffer(sizeof(float) * boxPositions.size(),
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, boxPositions.data());
    }

    // The PMREM pass draws a box through a classic vec3 position input, which the vertex
    // pulling pipeline above cannot express. Same render pass and dynamic state.
    // Records one PMREM box draw into whatever render pass is currently open. The
    // descriptor set is allocated per draw: the pool hands them out cheaply and the
    // equirectangular source only becomes known at draw time.
    void drawPmremFace(const TextureRef& source, const std::array<float, 48>& transforms) {
        VkDescriptorSet set = VK_NULL_HANDLE;
        for (;;) {
            if (activeDescriptorPool == descriptorPools.size()) {
                VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12288},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32768}};
                VkDescriptorPoolCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
                create.maxSets = 4096;
                create.poolSizeCount = 3;
                create.pPoolSizes = sizes;
                VkDescriptorPool pool;
                check(api.vkCreateDescriptorPool(device, &create, nullptr, &pool),
                      "vkCreateDescriptorPool(pmrem)");
                descriptorPools.push_back(pool);
            }
            VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocate.descriptorPool = descriptorPools[activeDescriptorPool];
            allocate.descriptorSetCount = 1;
            allocate.pSetLayouts = &pmremTextureLayout;
            const auto result = api.vkAllocateDescriptorSets(device, &allocate, &set);
            if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
                ++activeDescriptorPool;
                continue;
            }
            check(result, "vkAllocateDescriptorSets(pmrem)");
            break;
        }
        const VkDescriptorImageInfo image{source->sampler, source->image.view,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set;
        write.dstBinding = 1;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;
        api.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        api.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pmremPipelineHandle);
        const VkDeviceSize offset = 0;
        api.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &pmremVertices.buffer, &offset);
        api.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pmremPipelineLayout, 0, 1, &set, 0, nullptr);
        api.vkCmdPushConstants(commandBuffer, pmremPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(float) * transforms.size(), transforms.data());
        api.vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    }

    // The blur pass samples a cubemap through its cube view and writes the bound face.
    void drawPmremBlurFace(VkImageView sourceCubeView, const std::array<float, 49>& block) {
        VkDescriptorSet set = VK_NULL_HANDLE;
        for (;;) {
            if (activeDescriptorPool == descriptorPools.size()) {
                VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12288},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32768}};
                VkDescriptorPoolCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
                create.maxSets = 4096;
                create.poolSizeCount = 3;
                create.pPoolSizes = sizes;
                VkDescriptorPool pool;
                check(api.vkCreateDescriptorPool(device, &create, nullptr, &pool),
                      "vkCreateDescriptorPool(pmrem blur)");
                descriptorPools.push_back(pool);
            }
            VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocate.descriptorPool = descriptorPools[activeDescriptorPool];
            allocate.descriptorSetCount = 1;
            allocate.pSetLayouts = &pmremBlurTextureLayout;
            const auto result = api.vkAllocateDescriptorSets(device, &allocate, &set);
            if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
                ++activeDescriptorPool;
                continue;
            }
            check(result, "vkAllocateDescriptorSets(pmrem blur)");
            break;
        }
        const VkDescriptorImageInfo image{pmremSampler, sourceCubeView,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = set;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;
        api.vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        api.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pmremBlurPipelineHandle);
        const VkDeviceSize offset = 0;
        api.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &pmremVertices.buffer, &offset);
        api.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pmremBlurPipelineLayout, 0, 1, &set, 0, nullptr);
        api.vkCmdPushConstants(commandBuffer, pmremBlurPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(float) * block.size(), block.data());
        api.vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    }

    // Same fixed function state as the equirect pass, but with the blur program.
    void createPmremBlurPipeline() {
        const auto stages = pmremBlurModules->pipelineStages();
        VkVertexInputBindingDescription binding{0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attribute{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex.vertexBindingDescriptionCount = 1;
        vertex.pVertexBindingDescriptions = &binding;
        vertex.vertexAttributeDescriptionCount = 1;
        vertex.pVertexAttributeDescriptions = &attribute;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rasterization{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization.cullMode = VK_CULL_MODE_NONE;
        rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = VK_FALSE;
        depth.depthWriteEnable = VK_FALSE;
        VkPipelineColorBlendAttachmentState blend{};
        blend.blendEnable = VK_FALSE;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blend;
        const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamicStates;
        VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        create.stageCount = 2;
        create.pStages = stages.data();
        create.pVertexInputState = &vertex;
        create.pInputAssemblyState = &assembly;
        create.pViewportState = &viewport;
        create.pRasterizationState = &rasterization;
        create.pMultisampleState = &multisample;
        create.pDepthStencilState = &depth;
        create.pColorBlendState = &colorBlend;
        create.pDynamicState = &dynamic;
        create.layout = pmremBlurPipelineLayout;
        create.renderPass = renderPass;
        check(api.vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create, nullptr,
                                           &pmremBlurPipelineHandle),
              "vkCreateGraphicsPipelines(pmrem blur)");
    }

    void createPmremPipeline() {
        const auto stages = pmremModules->pipelineStages();
        VkVertexInputBindingDescription binding{0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attribute{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex.vertexBindingDescriptionCount = 1;
        vertex.pVertexBindingDescriptions = &binding;
        vertex.vertexAttributeDescriptionCount = 1;
        vertex.pVertexAttributeDescriptions = &attribute;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rasterization{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterization.polygonMode = VK_POLYGON_MODE_FILL;
        // Upstream draws the box from the inside with blending and depth off.
        rasterization.cullMode = VK_CULL_MODE_NONE;
        rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = VK_FALSE;
        depth.depthWriteEnable = VK_FALSE;
        VkPipelineColorBlendAttachmentState blend{};
        blend.blendEnable = VK_FALSE;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo colorBlend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blend;
        const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamicStates;
        VkGraphicsPipelineCreateInfo create{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        create.stageCount = 2;
        create.pStages = stages.data();
        create.pVertexInputState = &vertex;
        create.pInputAssemblyState = &assembly;
        create.pViewportState = &viewport;
        create.pRasterizationState = &rasterization;
        create.pMultisampleState = &multisample;
        create.pDepthStencilState = &depth;
        create.pColorBlendState = &colorBlend;
        create.pDynamicState = &dynamic;
        create.layout = pmremPipelineLayout;
        create.renderPass = renderPass;
        check(api.vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &create, nullptr,
                                           &pmremPipelineHandle),
              "vkCreateGraphicsPipelines(pmrem)");
    }

    void imageBarrier(VkImage image, VkImageAspectFlags aspect, VkImageLayout oldLayout,
                      VkImageLayout newLayout, VkAccessFlags sourceAccess,
                      VkAccessFlags destinationAccess, VkPipelineStageFlags sourceStage,
                      VkPipelineStageFlags destinationStage, std::uint32_t layer = 0) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask = sourceAccess;
        barrier.dstAccessMask = destinationAccess;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspect;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = layer;
        barrier.subresourceRange.layerCount = 1;
        api.vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0,
                                 nullptr, 1, &barrier);
    }
    void beginFrame(bool clearColorAttachment, bool clearDepthAttachment, const Color& color,
                    HIGH_PRECISION alpha, bool srgb) {
        activeDescriptorPool = 0;
        for (const auto pool : descriptorPools)
            check(api.vkResetDescriptorPool(device, pool, 0), "vkResetDescriptorPool");
        check(api.vkResetCommandBuffer(commandBuffer, 0), "vkResetCommandBuffer");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        check(api.vkBeginCommandBuffer(commandBuffer, &begin), "vkBeginCommandBuffer");
        if ((active->initializedLayers & (1u << activeCubeFace)) == 0) {
            imageBarrier(active->color.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0,
                         VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, activeCubeFace);
            imageBarrier(active->depth.image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0,
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                             VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        }
        VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        render.renderPass = renderPass;
        render.framebuffer = active->layers > 1 && active->faceFramebuffer ? active->faceFramebuffer : active->framebuffer;
        render.renderArea.extent = {active->width, active->height};
        api.vkCmdBeginRenderPass(commandBuffer, &render, VK_SUBPASS_CONTENTS_INLINE);
        recording = true;
        ++recordingEpoch;
        clearAttachments(clearColorAttachment || !active->initialized, clearDepthAttachment || !active->initialized,
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
                srgb ? linearToSRGB(color.r) : static_cast<float>(color.r);
            attachment.clearValue.color.float32[1] =
                srgb ? linearToSRGB(color.g) : static_cast<float>(color.g);
            attachment.clearValue.color.float32[2] =
                srgb ? linearToSRGB(color.b) : static_cast<float>(color.b);
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
        imageBarrier(active->color.image, VK_IMAGE_ASPECT_COLOR_BIT,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, activeCubeFace);
        VkBufferImageCopy copy{};
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.baseArrayLayer = activeCubeFace;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = {width, height, 1};
        api.vkCmdCopyImageToBuffer(commandBuffer, active->color.image,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &copy);
        imageBarrier(active->color.image, VK_IMAGE_ASPECT_COLOR_BIT,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                     VK_ACCESS_TRANSFER_READ_BIT,
                     VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, activeCubeFace);
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
        std::memcpy(active->pixels.data(), mapped, active->pixels.size());
        api.vkUnmapMemory(device, readback.memory);
        active->initialized = true;
        active->initializedLayers |= 1u << activeCubeFace;
    }
};

VulkanRenderer::VulkanRenderer(const VulkanRendererParameters& parameters)
    : impl(std::make_unique<Impl>()) {
    impl->initialize(parameters);
    auto modes = JSValue::object();
    modes.set("TRIANGLES", 4); modes.set("LINES", 1); modes.set("LINE_STRIP", 3);
    modes.set("LINE_LOOP", 2); modes.set("POINTS", 0);
    impl->drawInfo = std::make_unique<WebGLInfo>(modes);
    info = impl->drawInfo->object();
    impl->initializeObjectProtocols();
    impl->clippingProperties = std::make_unique<WebGLProperties>();
    impl->clipping = std::make_unique<WebGLClipping>(impl->clippingProperties->object());
}
VulkanRenderer::~VulkanRenderer() = default;
void VulkanRenderer::setSize(std::uint32_t width, std::uint32_t height) {
    if (impl->rendering)
        throw std::logic_error("setSize inside render or upload callbacks is not supported yet");
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
void VulkanRenderer::setRenderTarget(std::shared_ptr<RenderTarget> target,
                                     std::uint32_t activeCubeFace,
                                     std::uint32_t activeMipmapLevel) {
    if (impl->rendering)
        throw std::logic_error("setRenderTarget inside render callbacks is not supported yet");
    if (target) {
        if (!impl->renderTarget) { impl->savedWidth = impl->width; impl->savedHeight = impl->height; }
        impl->renderTarget = target;
        impl->useTarget(impl->renderTarget.get());
        impl->activeCubeFace = activeCubeFace;
        if (activeMipmapLevel != 0)
            throw std::logic_error("render target mip levels are not implemented yet");
        auto cube = std::dynamic_pointer_cast<WebGLCubeRenderTarget>(impl->renderTarget);
        impl->resize(static_cast<std::uint32_t>(impl->renderTarget->width),
                    static_cast<std::uint32_t>(impl->renderTarget->height), cube ? 6u : 1u);
        impl->selectFace(activeCubeFace);
    } else if (impl->renderTarget) {
        impl->renderTarget.reset();
        // Park the target before resizing back, otherwise resize rebuilds it at one
        // layer: the unbind path has no target to derive a layer count from.
        impl->useTarget(nullptr);
        impl->resize(impl->savedWidth, impl->savedHeight);
    }
}
std::shared_ptr<RenderTarget> VulkanRenderer::getRenderTarget() const { return impl->renderTarget; }
void VulkanRenderer::renderEquirectangularToCube(const std::shared_ptr<Texture>& source,
                                                 const std::shared_ptr<RenderTarget>& target) {
    if (!source) throw std::invalid_argument("equirectangular source needed");
    auto cube = std::dynamic_pointer_cast<WebGLCubeRenderTarget>(target);
    if (!cube) throw std::invalid_argument("only cube render targets have six faces");
    // Reuse the renderer's own upload, caching and mipmap path rather than assembling a
    // texture config here.
    std::shared_ptr<Impl::TextureEntry> binding;
    const auto texture = impl->updateTexture(source, binding);
    if (!texture) throw std::runtime_error("equirectangular source could not be uploaded");
    const auto previous = getRenderTarget();
    const auto previousFace = getActiveCubeFace();
    const auto previousMip = getActiveMipmapLevel();
    // The box is the same for every face; what differs is which inner wall the camera sees,
    // because the vertex stage derives vWorldDirection from the model matrix alone. The
    // verified CubeCamera rig supplies the six views and the projection, so this pass uses
    // exactly the orientations the rest of the engine already trusts.
    const float model[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    CubeCamera rig(0.1, 100, JSValue::object());
    rig.updateMatrixWorld();
    for (std::uint32_t face = 0; face < 6; ++face) {
        std::array<float, 48> block{};
        std::copy(std::begin(model), std::end(model), block.begin());
        auto eye = std::dynamic_pointer_cast<PerspectiveCamera>(rig.children[face]);
        if (!eye) throw std::runtime_error("cube camera rig needs six eyes");
        eye->updateMatrixWorld();
        const auto view = eye->matrixWorldInverse.elements();
        for (int i = 0; i < 16; ++i) block[16 + i] = static_cast<float>(view[static_cast<std::size_t>(i)]);
        // The rig uses a negative field of view to mark cube face projection, which flips
        // the matrix; use an explicit positive ninety degrees so the mapping is upright.
        PerspectiveCamera faceProjection(90, 1, 0.1, 100);
        const auto projection = faceProjection.projectionMatrix.elements();
        for (int i = 0; i < 16; ++i) block[32 + i] = static_cast<float>(projection[static_cast<std::size_t>(i)]);
        setRenderTarget(cube, face);
        impl->beginFrame(false, false, impl->activeClearColor, impl->activeClearAlpha,
                         impl->activeClearSRGB);
        impl->drawPmremFace(texture, block);
        impl->endFrame();
    }
    setRenderTarget(previous, previousFace, previousMip);
}
void VulkanRenderer::blurCube(const std::shared_ptr<RenderTarget>& source,
                              const std::shared_ptr<RenderTarget>& destination, double sigma) {
    auto sourceCube = std::dynamic_pointer_cast<WebGLCubeRenderTarget>(source);
    auto destinationCube = std::dynamic_pointer_cast<WebGLCubeRenderTarget>(destination);
    if (!sourceCube || !destinationCube)
        throw std::invalid_argument("blurCube needs cube render targets");
    const auto previous = getRenderTarget();
    const auto previousFace = getActiveCubeFace();
    const auto previousMip = getActiveMipmapLevel();
    setRenderTarget(destinationCube);
    const auto parkedSource = impl->parkedTargets.find(sourceCube.get());
    if (parkedSource == impl->parkedTargets.end() || !parkedSource->second)
        throw std::logic_error("blurCube needs the source to have been rendered into first");
    const auto sourceView = parkedSource->second->color.cubeView;
    if (!sourceView) throw std::logic_error("blurCube source has no cube view");
    CubeCamera rig(0.1, 100, JSValue::object());
    rig.updateMatrixWorld();
    PerspectiveCamera faceProjection(90, 1, 0.1, 100);
    const auto projection = faceProjection.projectionMatrix.elements();
    const float model[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    for (std::uint32_t face = 0; face < 6; ++face) {
        auto eye = std::dynamic_pointer_cast<PerspectiveCamera>(rig.children[face]);
        if (!eye) throw std::runtime_error("cube camera rig needs six eyes");
        eye->updateMatrixWorld();
        std::array<float, 49> block{};
        std::copy(std::begin(model), std::end(model), block.begin());
        const auto view = eye->matrixWorldInverse.elements();
        for (int i = 0; i < 16; ++i)
            block[16 + i] = static_cast<float>(view[static_cast<std::size_t>(i)]);
        for (int i = 0; i < 16; ++i)
            block[32 + i] = static_cast<float>(projection[static_cast<std::size_t>(i)]);
        block[48] = static_cast<float>(sigma);
        setRenderTarget(destinationCube, face);
        // Clear on open so the draw is what the per target readback observes.
        impl->beginFrame(true, false, Color(0, 0, 0), 1, false);
        if (face == 0) {
            for (std::uint32_t layer = 0; layer < parkedSource->second->layers; ++layer) {
                impl->imageBarrier(parkedSource->second->color.image, VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, layer);
            }
        }
        impl->drawPmremBlurFace(sourceView, block);
        impl->endFrame();
    }
    setRenderTarget(previous, previousFace, previousMip);
}

std::uint32_t VulkanRenderer::getActiveCubeFace() const { return impl->activeCubeFace; }
std::uint32_t VulkanRenderer::getActiveMipmapLevel() const { return impl->activeMipmapLevel; }
std::vector<std::uint8_t> VulkanRenderer::readRenderTargetPixels(RenderTarget& target, std::uint32_t x,
                                                                  std::uint32_t y, std::uint32_t width,
                                                                  std::uint32_t height) {
    // Each GPU target keeps its own copy, so a target can be read even after being parked.
    const Impl::GpuTarget* gt = nullptr;
    if (impl->activeKey == &target) {
        gt = impl->active.get();
    } else {
        const auto found = impl->parkedTargets.find(&target);
        if (found != impl->parkedTargets.end()) gt = found->second.get();
    }
    if (!gt || gt->pixels.empty())
        throw std::logic_error("readRenderTargetPixels: that target has no pixels yet");
    const auto& source = gt->pixels;
    const auto rowBytes = static_cast<std::size_t>(gt->width) * 4;
    if (width == 0 || height == 0 || x + width > gt->width || y + height > gt->height ||
        source.size() < rowBytes * gt->height)
        throw std::out_of_range("readRenderTargetPixels outside the render target");
    std::vector<std::uint8_t> out(static_cast<std::size_t>(width) * height * 4);
    for (std::uint32_t row = 0; row < height; ++row)
        std::memcpy(out.data() + static_cast<std::size_t>(row) * width * 4,
                    source.data() + static_cast<std::size_t>(y + row) * rowBytes +
                        static_cast<std::size_t>(x) * 4,
                    static_cast<std::size_t>(width) * 4);
    return out;
}
std::vector<std::uint8_t> VulkanRenderer::readPixels() const {
    if (impl->recording) {
        impl->endFrame();
        // Preserve attachments while subsequent callbacks and draws continue this frame.
        impl->beginFrame(false, false, impl->clearColor, impl->clearAlpha, srgbOutput);
    }
    // The pixels belong to the target that was rendered into; the wide member above is
    // not the one that gets written, so returning it would hand back a vector that was
    // never filled. A renderer with no target has nothing to give.
    if (!impl->active) return {};
    return impl->active->pixels;
}
const VulkanRendererInfo& VulkanRenderer::getInfo() const {
    const auto render = info.get("render");
    impl->info.frame = render.get("frame").toNumber();
    impl->info.calls = render.get("calls").toNumber();
    impl->info.triangles = render.get("triangles").toNumber();
    impl->info.lines = render.get("lines").toNumber();
    impl->info.points = render.get("points").toNumber();
    return impl->info;
}
std::vector<std::string> VulkanRenderer::getValidationMessages() const {
    std::lock_guard lock(impl->validationMutex);
    return impl->validationMessages;
}

void VulkanRenderer::render(Scene& scene, Camera& camera) {
    if (impl->rendering)
        throw std::logic_error(
            "Nested render inside render or upload callbacks is not supported yet");
    struct RenderGuard {
        bool& active;
        explicit RenderGuard(bool& active) : active(active) { active = true; }
        ~RenderGuard() { active = false; }
    } renderGuard(impl->rendering);
    // Hooks may detach the camera or release the caller's last managed scene reference.
    [[maybe_unused]] const auto sceneLifetime = scene.weak_from_this().lock();
    [[maybe_unused]] const auto cameraLifetime = camera.weak_from_this().lock();
    impl->pruneResources();
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
    impl->localClipping = localClippingEnabled;
    impl->clippingEnabled = impl->clipping->init(clippingPlanes, impl->localClipping);
    ClippingCameraView clippingCamera(camera, cameraLifetime);
    struct Item {
        std::shared_ptr<Object3D> object;
        std::shared_ptr<BufferGeometry> geometry;
        std::shared_ptr<Material> material;
        std::shared_ptr<BufferGeometry::Group> group;
        double groupOrder, renderOrder;
        HIGH_PRECISION z;
    };
    std::vector<Item> opaque, transparent;
    auto project = [&](auto&& self, Object3D& object, double groupOrder) -> void {
        [[maybe_unused]] const auto objectLifetime = object.weak_from_this().lock();
        if (!object.visible)
            return;
        if (object.layers.test(camera.layers)) {
            if (object.isGroup)
                groupOrder = object.renderOrder;
            else if (object.isLOD) {
                if (auto* lod = dynamic_cast<LOD*>(&object); lod && lod->autoUpdate.strictEquals(JSValue(true)))
                    lod->update(camera);
            }
            auto projectPrimitive = [&](auto& primitive) {
                if (object.frustumCulled && !primitive.intersectsFrustum(frustum)) return;
                auto geometry = primitive.geometry;
                if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'id')");
                const auto updateObject = [&]() {
                    if (!objectLifetime) throw std::invalid_argument("Scene objects must use shared_ptr ownership");
                    const auto updated = impl->objects->update(nativeObject(objectLifetime));
                    return std::static_pointer_cast<BufferGeometry>(updated.hostPayload("three.BufferGeometry"));
                };
                if constexpr (!std::is_same_v<std::remove_reference_t<decltype(primitive)>, Sprite>)
                    geometry = updateObject();
                Vector4 projected;
                if (sortObjects) {
                    if constexpr (std::is_same_v<std::remove_reference_t<decltype(primitive)>, Sprite>) {
                        projected.setFromMatrixPosition(object.matrixWorld);
                    } else {
                        if (!geometry->boundingSphere) geometry->computeBoundingSphere();
                        if (!geometry->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
                        projected.copy(geometry->boundingSphere->center).applyMatrix4(object.matrixWorld);
                    }
                    projected.applyMatrix4(viewProjection);
                }
                if constexpr (std::is_same_v<std::remove_reference_t<decltype(primitive)>, Sprite>)
                    geometry = updateObject();
                auto add = [&](const std::shared_ptr<Material>& material,
                               std::shared_ptr<BufferGeometry::Group> group) {
                    if (!material || !material->visible) return;
                    if (!objectLifetime) throw std::invalid_argument("Scene objects must use shared_ptr ownership");
                    Item item{objectLifetime, geometry, material, std::move(group), groupOrder,
                              object.renderOrder, projected.z * (camera.reversedDepth ? -1 : 1)};
                    (material->transparent ? transparent : opaque).push_back(std::move(item));
                };
                if constexpr (requires { primitive.materials; primitive.materialIsArray; }) {
                    if (primitive.materialIsArray || !primitive.materials.empty()) {
                        for (const auto& group : geometry->groups) {
                            if (!(group.materialIndex >= 0 && group.materialIndex < double(primitive.materials.size())) ||
                                std::floor(group.materialIndex) != group.materialIndex) continue;
                            add(primitive.materials[static_cast<std::size_t>(group.materialIndex)],
                                geometry->groups.owner(group));
                        }
                        return;
                    }
                }
                add(primitive.material, nullptr);
            };
            if (auto* line = dynamic_cast<Line*>(&object)) projectPrimitive(*line);
            else if (auto* points = dynamic_cast<Points*>(&object)) projectPrimitive(*points);
            else if (auto* sprite = dynamic_cast<Sprite*>(&object)) projectPrimitive(*sprite);
            if (auto* mesh = dynamic_cast<Mesh*>(&object); mesh && mesh->geometry) {
                if (!object.frustumCulled || mesh->intersectsFrustum(frustum)) {
                    const auto updated = impl->objects->update(nativeObject(objectLifetime));
                    const auto geometry = std::static_pointer_cast<BufferGeometry>(updated.hostPayload("three.BufferGeometry"));
                    if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'id')");
                    if (mesh->isInstancedMesh) {
                        const auto instanced = std::dynamic_pointer_cast<InstancedMesh>(objectLifetime);
                        if (!instanced) throw std::invalid_argument("InstancedMesh requires a managed native instance");
                        impl->registerInstance(instanced);
                    }
                    Vector4 projected;
                    if (sortObjects) {
                        if (auto* instanced = dynamic_cast<InstancedMesh*>(mesh)) {
                            if (!instanced->boundingSphere) instanced->computeBoundingSphere();
                            if (!instanced->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
                            projected.copy(instanced->boundingSphere->center);
                        } else if (auto* skinned = dynamic_cast<SkinnedMesh*>(mesh)) {
                            if (!skinned->boundingSphere) skinned->computeBoundingSphere();
                            if (!skinned->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
                            projected.copy(skinned->boundingSphere->center);
                        } else {
                            if (!geometry->boundingSphere) geometry->computeBoundingSphere();
                            if (!geometry->boundingSphere) throw JSTypeError("Cannot read properties of null (reading 'center')");
                            projected.copy(geometry->boundingSphere->center);
                        }
                        projected.applyMatrix4(object.matrixWorld).applyMatrix4(viewProjection);
                    }
                    const auto z = projected.z * (camera.reversedDepth ? -1 : 1);
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
                                  geometry,
                                  material,
                                  group,
                                  groupOrder,
                                  mesh->renderOrder,
                                  z};
                        (material->transparent ? transparent : opaque)
                            .push_back(std::move(item));
                    };
                    if (mesh->materialIsArray || !mesh->materials.empty()) {
                        for (const auto& group : geometry->groups) {
                            if (!(group.materialIndex >= 0 && group.materialIndex < double(mesh->materials.size())) ||
                                std::floor(group.materialIndex) != group.materialIndex)
                                continue;
                            add(mesh->materials[static_cast<std::size_t>(group.materialIndex)],
                                geometry->groups.owner(group));
                        }
                    } else
                        add(mesh->material, nullptr);
                }
            }
        }
        const auto childCount = object.children.size();
        for (std::size_t i = 0; i < childCount; ++i) {
            if (i >= object.children.size())
                throw std::invalid_argument("Scene children changed during attribute upload traversal");
            const auto child = object.children[i];
            if (child)
                self(self, *child, groupOrder);
        }
    };
    project(project, scene, 0);
    if (sortObjects) {
        v8ArraySort(opaque, [](const Item& a, const Item& b) -> double {
            if (a.groupOrder != b.groupOrder)
                return a.groupOrder - b.groupOrder;
            if (a.renderOrder != b.renderOrder)
                return a.renderOrder - b.renderOrder;
            if (a.material->id != b.material->id)
                return double(a.material->id) - double(b.material->id);
            if (a.z != b.z)
                return a.z - b.z;
            return double(a.object->id) - double(b.object->id);
        });
        v8ArraySort(transparent, [](const Item& a, const Item& b) -> double {
            if (a.groupOrder != b.groupOrder)
                return a.groupOrder - b.groupOrder;
            if (a.renderOrder != b.renderOrder)
                return a.renderOrder - b.renderOrder;
            if (a.z != b.z)
                return b.z - a.z;
            return double(a.object->id) - double(b.object->id);
        });
    }
    // Retain uploaded buffers until every recorded draw has completed on the GPU.
    struct Prepared {
        Impl::BufferRef positions, colors, fog, indices, mapUv, alphaUv, textureUniforms;
        Impl::BufferRef instanceMatrices, instanceColors, morphPositions, clippingPlanes;
        Impl::TextureRef map, alpha, morphWeights;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
        VkPipeline pipeline;
        PushConstants constants;
        std::uint32_t start, count;
        double logicalCount;
        JSValue logicalInstanceCount{1};
        std::uint32_t instanceCount = 1;
        bool skipInstances = false;
        bool invalidBindings = false;
        bool indexed, wireframe, colorWrite;
    };
    std::vector<Prepared> prepared;
    auto cleanup = [&]() {
        prepared.clear();
    };
    try {
        auto renderInfo = info.get("render");
        renderInfo.set("frame", renderInfo.get("frame").toNumber() + 1);
        if (info.get("autoReset").strictEquals(true)) {
            const auto reset = info.get("reset");
            reset.call(info);
        }
        if (impl->clippingEnabled.strictEquals(true)) impl->clipping->beginShadows();
        // Shadow rendering is not part of the current MeshBasic path.
        if (impl->clippingEnabled.strictEquals(true)) impl->clipping->endShadows();
        if (impl->clippingEnabled.strictEquals(true)) impl->clipping->setGlobalState(clippingPlanes, clippingCamera.value);
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
            // Capability checks belong to submitted render items: visibility, culling,
            // material visibility and the late layer check have already filtered the list.
            if (dynamic_cast<Line*>(item.object.get()))
                throw std::invalid_argument("Vulkan Line rendering is not implemented yet");
            if (dynamic_cast<Points*>(item.object.get()))
                throw std::invalid_argument("Vulkan Points rendering is not implemented yet");
            if (dynamic_cast<Sprite*>(item.object.get()))
                throw std::invalid_argument("Vulkan Sprite rendering is not implemented yet");
            const auto& mesh = *static_cast<Mesh*>(item.object.get());
            const auto* instanced = mesh.isInstancedMesh ? dynamic_cast<const InstancedMesh*>(&mesh) : nullptr;
            if (mesh.isSkinnedMesh)
                throw std::invalid_argument("Vulkan SkinnedMesh rendering is not implemented yet");
            auto basic = std::dynamic_pointer_cast<MeshBasicMaterial>(effectiveMaterial);
            if (!basic)
                throw std::invalid_argument(
                    "VulkanRenderer currently supports MeshBasicMaterial only");
            const auto& material = *basic;
            if (material.lightMap || material.aoMap || material.specularMap || material.envMap)
                throw std::invalid_argument("Vulkan lightMap, aoMap, specularMap and envMap sampling are not implemented yet");
            if (material.alphaHash || material.alphaToCoverage || material.stencilWrite || material.dithering)
                throw std::invalid_argument("Vulkan alphaHash, alphaToCoverage, stencil and dithering are not implemented yet");
            const auto managedMaterial = nativeMaterial(basic);
            const auto materialProperties = impl->clippingProperties->get(managedMaterial);
            const bool sameCamera = &camera == impl->currentCamera && camera.id == impl->currentCameraId &&
                (clippingCamera.borrow ? impl->currentCameraOwner.isUndefined()
                                      : clippingCamera.value.sameValue(impl->currentCameraOwner));
            if (impl->clippingEnabled.strictEquals(true) &&
                (impl->localClipping.strictEquals(true) || !sameCamera)) {
                const bool useCache = sameCamera &&
                    managedMaterial.get("id").strictEquals(impl->currentMaterialId);
                impl->clipping->setState(managedMaterial, clippingCamera.value, useCache);
            }
            impl->registerClippingMaterial(basic, materialProperties);
            if (!managedMaterial.get("id").strictEquals(impl->currentMaterialId))
                impl->currentMaterialId = managedMaterial.get("id");
            impl->currentCamera = &camera;
            impl->currentCameraId = camera.id;
            impl->currentCameraOwner = clippingCamera.borrow ? JSValue() : clippingCamera.value;
            const auto planeCount = impl->clipping->numPlanes.toNumber();
            const auto intersectionCount = impl->clipping->numIntersection.toNumber();
            if (!std::isfinite(planeCount) || planeCount < 0 || std::floor(planeCount) != planeCount ||
                !std::isfinite(intersectionCount) || intersectionCount < 0 || intersectionCount > planeCount ||
                std::floor(intersectionCount) != intersectionCount ||
                16 + planeCount * 16 > impl->maxStorageBufferRange)
                throw std::invalid_argument("Clipping planes exceed Vulkan storage limits");
            const auto numPlanes = static_cast<std::uint32_t>(planeCount);
            const auto numIntersection = static_cast<std::uint32_t>(intersectionCount);
            std::vector<std::byte> clippingData(16 + numPlanes * 16);
            const std::uint32_t clippingHeader[] = {numPlanes, numPlanes - numIntersection, 0, 0};
            std::memcpy(clippingData.data(), clippingHeader, sizeof(clippingHeader));
            if (numPlanes) {
                const auto uniform = impl->clipping->uniform.value();
                const auto source = NativeArray_detail::unwrap(uniform.get("value"));
                const auto* values = std::get_if<Float32Array>(&source.storage());
                if (!values || values->size() < numPlanes * 4)
                    throw JSTypeError("Clipping uniform must contain its projected Float32 planes");
                for (std::size_t i = 0; i < numPlanes * 4; ++i) {
                    const float value = (*values)[i];
                    std::memcpy(clippingData.data() + 16 + i * sizeof(float), &value, sizeof(value));
                }
            }
            if (material.polygonOffset || (material.wireframe && material.wireframeLinewidth != 1))
                throw std::invalid_argument(
                    "Vulkan polygon offset and wide wireframe lines are not implemented yet");
            if ((material.blending == 3 || material.blending == 4) && !material.premultipliedAlpha)
                throw std::invalid_argument(
                    "three.js requires premultipliedAlpha for subtractive and multiply blending");
            if (item.geometry->morphAttributes.contains("color"))
                throw std::invalid_argument("Vulkan morph colors are not implemented yet");
            const auto geometryCache = impl->registerGeometry(item.geometry);
            const auto morph = item.geometry->morphAttributes.find("position");
            const bool hasMorph = morph != item.geometry->morphAttributes.end();
            if (hasMorph && item.geometry != mesh.geometry)
                throw std::invalid_argument("Changing mesh.geometry during morph rendering is not supported yet");
            if (hasMorph && (!geometryCache->morphInitialized ||
                            geometryCache->morphPositions.size() != morph->second.size())) {
                geometryCache->morphPositions.clear();
                for (const auto& attribute : morph->second)
                    geometryCache->morphPositions.emplace_back(*ordinaryAttribute(attribute));
                geometryCache->morphBuffer.reset();
                const auto sourcePosition = item.geometry->getAttribute("position");
                const auto vertexCount = sourcePosition ? sourcePosition->getCount() : 0;
                if (!std::isfinite(vertexCount) || vertexCount < 0 || vertexCount > UINT32_MAX)
                    throw std::invalid_argument("Morph texture exceeds Vulkan buffer limits");
                geometryCache->morphVertexCount = static_cast<std::uint32_t>(std::ceil(vertexCount));
                geometryCache->morphInitialized = true;
            }
            const auto morphTexture = instanced && hasMorph ? instanced->morphTexture : nullptr;
            if (instanced && hasMorph && !morphTexture && !mesh.morphTargetInfluences)
                throw JSTypeError("Cannot read properties of undefined (reading 'length')");
            std::shared_ptr<Impl::TextureEntry> morphBinding;
            // WebGLRenderer uploads per-object morph uniforms before material uniforms.
            auto morphResource = impl->updateTexture(morphTexture, morphBinding);
            const bool fogEnabled = scene.fog && material.fog;
            PushConstants uniformSnapshot{};
            uniformSnapshot.diffuse[0] = static_cast<float>(material.color.r);
            uniformSnapshot.diffuse[1] = static_cast<float>(material.color.g);
            uniformSnapshot.diffuse[2] = static_cast<float>(material.color.b);
            uniformSnapshot.diffuse[3] = static_cast<float>(material.opacity);
            uniformSnapshot.settings[0] = static_cast<float>(material.alphaTest);
            uniformSnapshot.settings[1] =
                !material.transparent && material.blending == 1 && !material.alphaToCoverage ? 1.0f
                                                                                             : 0.0f;
            uniformSnapshot.settings[2] = material.premultipliedAlpha ? 1.0f : 0.0f;
            uniformSnapshot.settings[3] = srgbOutput ? 1.0f : 0.0f;
            if (fogEnabled) {
                const Color* fogColor;
                if (auto fog = std::dynamic_pointer_cast<Fog>(scene.fog)) {
                    fogColor = &fog->color;
                    uniformSnapshot.fogParams[0] = 1;
                    uniformSnapshot.fogParams[1] = static_cast<float>(fog->near);
                    uniformSnapshot.fogParams[2] = static_cast<float>(fog->far);
                } else if (auto fog = std::dynamic_pointer_cast<FogExp2>(scene.fog)) {
                    fogColor = &fog->color;
                    uniformSnapshot.fogParams[0] = 2;
                    uniformSnapshot.fogParams[1] = static_cast<float>(fog->density);
                } else
                    throw std::invalid_argument("Unknown scene fog type");
                uniformSnapshot.fogColor[0] =
                    srgbOutput ? linearToSRGB(fogColor->r) : static_cast<float>(fogColor->r);
                uniformSnapshot.fogColor[1] =
                    srgbOutput ? linearToSRGB(fogColor->g) : static_cast<float>(fogColor->g);
                uniformSnapshot.fogColor[2] =
                    srgbOutput ? linearToSRGB(fogColor->b) : static_cast<float>(fogColor->b);
            }
            const auto mapTexture = material.map, alphaTexture = material.alphaMap;
            Impl::TextureUniforms textureUniforms;
            const auto matrix = [](const std::shared_ptr<Texture>& texture, float* destination) {
                Matrix3 identity;
                if (texture && texture->matrixAutoUpdate) texture->updateMatrix();
                const auto elements = texture ? texture->matrix.elements() : identity.elements();
                for (int column = 0; column < 3; ++column)
                    for (int row = 0; row < 3; ++row)
                        destination[column * 4 + row] = float(elements[column * 3 + row]);
            };
            // Snapshot both transforms before upload callbacks, as refreshMaterialUniforms does.
            matrix(mapTexture, textureUniforms.mapTransform);
            matrix(alphaTexture, textureUniforms.alphaTransform);
            const auto uvAttribute = [&](const std::shared_ptr<Texture>& texture) -> std::shared_ptr<VertexAttribute> {
                if (!texture) return nullptr;
                if (!std::isfinite(texture->channel) || texture->channel < 0 || texture->channel > 3 ||
                    std::trunc(texture->channel) != texture->channel)
                    throw std::invalid_argument("Vulkan texture channel must select uv, uv1, uv2 or uv3");
                return item.geometry->getAttribute(texture->channel == 0 ? "uv" : "uv" + std::to_string(int(texture->channel)));
            };
            const auto mapUv = uvAttribute(mapTexture), alphaUv = uvAttribute(alphaTexture);
            std::shared_ptr<Impl::TextureEntry> mapBinding, alphaBinding;
            auto mapResource = impl->updateTexture(mapTexture, mapBinding);
            auto alphaResource = impl->updateTexture(alphaTexture, alphaBinding);
            // Uploading alphaMap may update the very same Source/cache-key storage bound as map.
            if (mapBinding && mapBinding->resource) mapResource = mapBinding->resource;
            if (alphaBinding && alphaBinding->resource) alphaResource = alphaBinding->resource;
            if (morphBinding && morphBinding->resource) morphResource = morphBinding->resource;
            const Matrix4& modelView = mesh.modelViewMatrix;
            const auto position = item.geometry->getAttribute("position");
            if (!position)
                return;

            const auto index = item.geometry->index;
            auto effectiveIndex = index;
            std::vector<std::uint32_t> indices;
            Impl::BufferRef indexBuffer;
            if (material.wireframe) {
                const auto wire = impl->geometries->getWireframeAttribute(nativeGeometry(item.geometry));
                if (wire.isUndefined()) return;
                effectiveIndex = std::dynamic_pointer_cast<BufferAttribute>(::vertexAttribute(wire));
            }
            const double factor = material.wireframe ? 2 : 1;
            const double geometryCount = effectiveIndex ? effectiveIndex->count : position->getCount();
            double start = double(item.geometry->drawRange.start) * factor;
            double end = (double(item.geometry->drawRange.start) +
                          double(item.geometry->drawRange.count)) * factor;
            if (item.group) {
                const auto& group = *item.group;
                start = MathUtils::max(start, double(group.start) * factor);
                end = MathUtils::min(end, (double(group.start) + double(group.count)) * factor);
            }
            end = MathUtils::min(end, geometryCount);
            // three.js submits a zero-count draw. Only a negative range skips binding/setup.
            if (end < start || std::isinf(end - start))
                return;
            const auto count = end - start;
            if (!std::isfinite(count))
                throw std::invalid_argument("Non-finite Vulkan draw count");
            if (start < 0 || start > UINT32_MAX || count > UINT32_MAX)
                throw std::invalid_argument("Geometry exceeds Vulkan draw limits");
            if ((index || material.wireframe) && std::floor(start) != start)
                throw std::invalid_argument("Indexed draw offset must be aligned to its index elements");
            Prepared draw{};
            draw.map = mapResource; draw.alpha = alphaResource;
            draw.start = static_cast<std::uint32_t>(start);
            draw.count = static_cast<std::uint32_t>(count);
            draw.logicalCount = count;
            draw.indexed = index != nullptr || material.wireframe;
            draw.wireframe = material.wireframe;
            draw.colorWrite = material.colorWrite;
            draw.morphWeights = morphResource;
            const auto colors = material.vertexColors ? item.geometry->getAttribute("color") : nullptr;
            const bool programHasInstanceColor = instanced && instanced->instanceColor != nullptr;
            const auto program = impl->bindingProgram(material, item.geometry, instanced, mapTexture, alphaTexture,
                material.vertexColors, numPlanes, numIntersection);
            impl->bindingStates->setup(nativeObject(item.object), nativeMaterial(basic), program,
                nativeGeometry(item.geometry), effectiveIndex ? nativeAttribute(effectiveIndex) : JSValue(nullptr));
            auto bindings = impl->bindingCommands->snapshot();
            bool validBindings = true;
            const auto activeLocation=[&](unsigned location) {
                return location==0 || (location==1 && material.vertexColors) ||
                    (location==3 && bool(mapTexture)) || (location==4 && bool(alphaTexture) && (!mapTexture || mapTexture->channel!=alphaTexture->channel)) ||
                    (instanced && location>=5 && location<=8) || (location==9 && programHasInstanceColor);
            };
            for(unsigned location=0;location<bindings.attributes.size();++location) {
                const auto& input=bindings.attributes[location];
                if(activeLocation(location) && input.enabled &&
                    (!impl->attributeCommands->hasStorage(input.buffer) || input.integer))validBindings=false;
            }
            if(draw.indexed && !impl->attributeCommands->hasStorage(bindings.index))validBindings=false;
            // An enabled vertex array without a buffer is a WebGL draw error.
            // Preserve callback/statistic flow while submitting no unsafe fetch.
            if(!validBindings) {
                impl->bindingCommands->invalidDraw();
                draw.invalidBindings=true;
                if(instanced)draw.logicalInstanceCount=mesh.count.asValue();
                else if(item.geometry->isInstancedBufferGeometry)
                    draw.logicalInstanceCount=geometryInstanceCount(item.geometry);
                draw.skipInstances=draw.logicalInstanceCount.strictEquals(0);
                prepared.push_back(std::move(draw));return;
            }
            if (effectiveIndex) {
                const auto cachedIndex = impl->decodeAttribute(effectiveIndex, true,bindings.index);
                indices = impl->indexValues(*cachedIndex->snapshot);
                indexBuffer = cachedIndex->indices;
            }
            if (draw.indexed && (draw.start > indices.size() ||
                                draw.count > indices.size() - draw.start)) {
                impl->bindingCommands->invalidDraw();draw.invalidBindings=true;
                if(instanced)draw.logicalInstanceCount=mesh.count.asValue();
                else if(item.geometry->isInstancedBufferGeometry)
                    draw.logicalInstanceCount=geometryInstanceCount(item.geometry);
                draw.skipInstances=draw.logicalInstanceCount.strictEquals(0);
                prepared.push_back(std::move(draw));return;
            }
            const auto positionCache = impl->boundInput(bindings.attributes[0], std::max(3u, bindings.attributes[0].size));
            const auto colorCache = material.vertexColors ? impl->boundInput(bindings.attributes[1], colors && colors->getItemSize()==4 ? 4 : 3) : nullptr;
            const auto mapLocation = mapTexture ? 3u : 4u;
            const auto alphaLocation = alphaTexture && mapTexture && alphaTexture->channel == mapTexture->channel ? 3u : 4u;
            const auto mapCache = mapTexture ? impl->boundInput(bindings.attributes[mapLocation], 2) : nullptr;
            const auto alphaCache = alphaTexture ? impl->boundInput(bindings.attributes[alphaLocation], 2) : nullptr;
            if (!impl->emptyStorage) {
                constexpr float defaults[] = {0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1};
                impl->emptyStorage = impl->sharedBuffer(sizeof(defaults), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, defaults);
            }
            draw.instanceMatrices = draw.instanceColors = draw.morphPositions = impl->emptyStorage;
            std::shared_ptr<Impl::AttributeCache> matrixCache, instanceColorCache;
            std::uint32_t matrixDivisor = 1, colorDivisor = 1;
            if (instanced) {
                const bool hasInstanceColor = programHasInstanceColor;
                matrixCache = impl->boundMatrix(bindings);
                instanceColorCache = hasInstanceColor ? impl->boundInput(bindings.attributes[9],3) : nullptr;
                matrixDivisor=bindings.attributes[5].divisor;
                colorDivisor=bindings.attributes[9].divisor;
                draw.instanceMatrices=matrixCache->vertices[false];
                if(instanceColorCache)draw.instanceColors=instanceColorCache->vertices[false];
                textureUniforms.instance[0]=1;textureUniforms.instance[1]=hasInstanceColor?1:0;
                textureUniforms.instance[2]=matrixDivisor;textureUniforms.instance[3]=colorDivisor;
                textureUniforms.instanceLayout[0]=16;
                textureUniforms.instanceLayout[1]=instanceColorCache?3:0;
                for(unsigned col=0;col<4;++col)textureUniforms.matrixDivisors[col]=bindings.attributes[5+col].divisor;
                for(int i=0;i<16;++i)textureUniforms.modelView[i]=float(modelView.elements()[i]);
                draw.logicalInstanceCount=mesh.count.asValue();
                draw.skipInstances=draw.logicalInstanceCount.strictEquals(JSValue(0));
                if(!draw.skipInstances) {
                    const auto converted=unsignedLong(draw.logicalInstanceCount.toNumber());
                    if(converted>INT32_MAX)throw std::invalid_argument("Negative WebGL instance count cannot be submitted to Vulkan");
                    draw.instanceCount=converted;
                } else draw.instanceCount=0;
            }
            else if(item.geometry->isInstancedBufferGeometry) {
                draw.logicalInstanceCount=geometryInstanceCount(item.geometry);
                draw.skipInstances=draw.logicalInstanceCount.strictEquals(0);
                draw.instanceCount=draw.skipInstances?0:unsignedLong(draw.logicalInstanceCount.toNumber());
                if(draw.instanceCount>INT32_MAX)throw std::invalid_argument("Negative WebGL instance count cannot be submitted to Vulkan");
            }
            const bool deform = hasMorph && !morphTexture && mesh.morphTargetInfluences;
            if(deform && bindings.attributes[0].divisor!=0)
                throw std::invalid_argument("Per-instance position attributes with uniform morph weights require the GPU morph path, which is not implemented yet");
            if (deform && draw.count > 0 && mesh.morphTargetInfluences->size() < geometryCache->morphPositions.size())
                throw std::invalid_argument("Short morph influence arrays require retained WebGL uniform state, which is not implemented");
            // Vulkan may fetch every submitted vertex, including an incomplete primitive.
            // Validate only those fetches against uploaded storage, not all attribute rows.
            const auto fetchedVertex = [&](std::size_t offset) -> std::size_t {
                return draw.indexed ? indices[std::size_t(draw.start) + offset]
                                    : std::size_t(draw.start) + offset;
            };
            const auto availableVertices = [](const auto& snapshot, std::size_t components) {
                if(snapshot.constantInput)return SIZE_MAX;
                if(snapshot.boundView)return std::size_t(snapshot.count);
                const auto size = Impl::componentCount(snapshot);
                return size < components ? std::size_t(0)
                    : std::size_t(std::floor(double(size - components) / snapshot.itemSize)) + 1;
            };
            const auto availablePositions = availableVertices(*positionCache->snapshot, 3);
            const auto inputRow = [&](unsigned location,std::size_t vertex,std::uint32_t instance)->std::size_t {
                const auto& input=bindings.attributes[location];
                if(!input.enabled)return 0;
                return input.divisor?instance/input.divisor:vertex;
            };
            const auto availableColors = colorCache ? availableVertices(*colorCache->snapshot,
                colorCache->snapshot->itemSize == 4 ? 4 : 3) : 0;
            const auto availableMapUv = mapCache ? availableVertices(*mapCache->snapshot, 2) : 0;
            const auto availableAlphaUv = alphaCache ? availableVertices(*alphaCache->snapshot, 2) : 0;
            std::size_t vertexCount = 0;
            bool invalidFetch=false;
            const auto fetchCount = draw.instanceCount == 0 ? 0u : draw.count;
            for (std::size_t offset = 0; offset < fetchCount; ++offset) {
                const auto vertex = fetchedVertex(offset);
                const auto instance=draw.instanceCount?draw.instanceCount-1:0;
                if (inputRow(0,vertex,instance) >= availablePositions ||
                    (colorCache && inputRow(1,vertex,instance) >= availableColors))
                    invalidFetch=true;
                if ((mapCache && inputRow(mapLocation,vertex,instance) >= availableMapUv) ||
                    (alphaCache && inputRow(alphaLocation,vertex,instance) >= availableAlphaUv))
                    invalidFetch=true;
                vertexCount = std::max(vertexCount, vertex + 1);
            }
            if (instanced && fetchCount > 0) {
                const auto checkInstances = [&](const auto& cache, std::uint32_t divisor, std::size_t components) {
                    if (!cache) return;
                    const auto rows = divisor == 0 ? vertexCount : (std::uint64_t(draw.instanceCount) - 1) / divisor + 1;
                    if (rows > (cache->snapshot->boundView ? cache->snapshot->count : Impl::componentCount(*cache->snapshot) / components))
                        invalidFetch=true;
                };
                for (const auto divisor : textureUniforms.matrixDivisors) checkInstances(matrixCache, divisor, 16);
                checkInstances(instanceColorCache, colorDivisor, 3);
            }
            if(invalidFetch) {
                impl->bindingCommands->invalidDraw();draw.invalidBindings=true;
                prepared.push_back(std::move(draw));return;
            }
            if (morphTexture) {
                if (!geometryCache->morphBuffer) {
                    const auto rows = geometryCache->morphVertexCount;
                    if (std::uint64_t(rows) * geometryCache->morphPositions.size() * 4 * sizeof(float) > impl->maxStorageBufferRange)
                        throw std::invalid_argument("Morph target storage exceeds the Vulkan device range limit");
                    std::vector<float> data(std::size_t(rows) * geometryCache->morphPositions.size() * 4, 0);
                    for (std::size_t target = 0; target < geometryCache->morphPositions.size(); ++target) {
                        const auto& attribute = geometryCache->morphPositions[target];
                        for (std::size_t row = 0; row < rows && double(row) < attribute.count; ++row)
                            for (std::size_t component = 0; component < 3; ++component)
                                data[(target * rows + row) * 4 + component] = float(attribute.getComponent(double(row),double(component)));
                    }
                    geometryCache->morphBuffer = impl->sharedBuffer(std::max<std::size_t>(data.size(),4) * sizeof(float),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, data.empty() ? nullptr : data.data());
                }
                if (fetchCount && vertexCount > geometryCache->morphVertexCount)
                    throw std::invalid_argument("Instanced morph fetch exceeds its uploaded target array");
                draw.morphPositions = geometryCache->morphBuffer;
                textureUniforms.instanceLayout[2] = std::uint32_t(geometryCache->morphPositions.size());
                textureUniforms.instanceLayout[3] = geometryCache->morphVertexCount;
            }
            std::vector<float> deformedPositions(deform ? vertexCount * 3 : 0);
            std::vector<float> fogDepths(fogEnabled ? vertexCount : 0);
            for (std::size_t offset = 0; offset < fetchCount; ++offset) {
                const auto i = fetchedVertex(offset);
                const auto& snapshot = *positionCache->snapshot;
                const auto sourceRow=inputRow(0,i,0);
                Vector3 deformed(snapshot.getX(sourceRow), snapshot.getY(sourceRow), snapshot.getZ(sourceRow));
                if (deform) {
                    Vector3 displacement;
                    for (std::size_t j = 0; j < geometryCache->morphPositions.size(); ++j) {
                        const auto influences = mesh.morphTargetInfluences->storage();
                        const auto influence = influences->holes.contains(j)
                            ? std::numeric_limits<double>::quiet_NaN() : influences->values[j];
                        if (influence == 0)
                            continue;
                        const auto& attribute = geometryCache->morphPositions[j];
                        Vector3 target(attribute.getX(i), attribute.getY(i), attribute.getZ(i));
                        if (!item.geometry->morphTargetsRelative)
                            target.sub(deformed);
                        displacement.addScaledVector(target, influence);
                    }
                    deformed.add(displacement);
                }
                if (!std::isfinite(deformed.x) || !std::isfinite(deformed.y) ||
                    !std::isfinite(deformed.z))
                    throw std::invalid_argument("Non-finite geometry position");
                if (deform) {
                    deformedPositions[i * 3] = static_cast<float>(deformed.x);
                    deformedPositions[i * 3 + 1] = static_cast<float>(deformed.y);
                    deformedPositions[i * 3 + 2] = static_cast<float>(deformed.z);
                }
                if (fogEnabled)
                    fogDepths[i] = static_cast<float>(-transform(modelView, deformed.x,
                                                               deformed.y, deformed.z)[2]);
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
            std::copy(std::begin(uniformSnapshot.diffuse), std::end(uniformSnapshot.diffuse), draw.constants.diffuse);
            std::copy(std::begin(uniformSnapshot.settings), std::end(uniformSnapshot.settings), draw.constants.settings);
            std::copy(std::begin(uniformSnapshot.fogColor), std::end(uniformSnapshot.fogColor), draw.constants.fogColor);
            std::copy(std::begin(uniformSnapshot.fogParams), std::end(uniformSnapshot.fogParams), draw.constants.fogParams);
            Matrix4 world;
            world.copy(mesh.matrixWorld);
            const auto positionStride = deform ? 3 * sizeof(float) : positionCache->snapshot->itemSize * sizeof(float);
            if (positionStride > UINT32_MAX || (colorCache && colorCache->snapshot->itemSize > UINT32_MAX / sizeof(float)))
                throw std::invalid_argument("Attribute itemSize exceeds Vulkan vertex binding limits");
            if ((mapCache && mapCache->snapshot->itemSize > UINT32_MAX / sizeof(float)) ||
                (alphaCache && alphaCache->snapshot->itemSize > UINT32_MAX / sizeof(float)))
                throw std::invalid_argument("UV itemSize exceeds Vulkan vertex binding limits");
            draw.pipeline =
                impl->pipeline(material, side, world.determinant() < 0, camera.reversedDepth,
                               static_cast<std::uint32_t>(positionStride),
                               colorCache ? static_cast<std::uint32_t>(colorCache->snapshot->itemSize) : 4,
                colorCache != nullptr && bindings.attributes[1].enabled, fogEnabled,
                               mapCache ? std::uint32_t(mapCache->snapshot->itemSize) : 2,
                               alphaCache ? std::uint32_t(alphaCache->snapshot->itemSize) : 2,
                               bool(mapCache) && bindings.attributes[mapLocation].enabled,
                               bool(alphaCache) && bindings.attributes[alphaLocation].enabled);
            draw.positions = deform
                                 ? impl->sharedBuffer(
                                       std::max<std::size_t>(deformedPositions.size(), 1) * sizeof(float),
                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       deformedPositions.empty() ? nullptr : deformedPositions.data())
                                 : positionCache->vertices[false];
            if (!impl->whiteColor) {
                constexpr float white[] = {1, 1, 1, 1};
                impl->whiteColor = impl->sharedBuffer(sizeof(white), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, white);
            }
            if (!impl->noFog) {
                constexpr float zero = 0;
                impl->noFog = impl->sharedBuffer(sizeof(zero), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &zero);
            }
            draw.colors = colorCache ? colorCache->vertices[false] : impl->whiteColor;
            draw.fog = fogEnabled
                           ? impl->sharedBuffer(std::max<std::size_t>(fogDepths.size(), 1) * sizeof(float),
                                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                fogDepths.empty() ? nullptr : fogDepths.data())
                           : impl->noFog;
            if (!impl->zeroUv) {
                constexpr float zero[] = {0, 0};
                impl->zeroUv = impl->sharedBuffer(sizeof(zero), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, zero);
            }
            draw.mapUv = mapCache ? mapCache->vertices[false] : impl->zeroUv;
            draw.alphaUv = alphaCache ? alphaCache->vertices[false] : impl->zeroUv;
            for (const auto& buffer : {draw.instanceMatrices, draw.instanceColors, draw.morphPositions,draw.positions,draw.colors,draw.mapUv,draw.alphaUv})
                if (buffer->size > impl->maxStorageBufferRange)
                    throw std::invalid_argument("Instance storage exceeds the Vulkan device range limit");
            const std::array<unsigned,4> locations{0,1,mapLocation,alphaLocation};
            const std::array<std::uint32_t,4> strides{deform?3u:std::uint32_t(positionCache->snapshot->itemSize),
                colorCache?std::uint32_t(colorCache->snapshot->itemSize):4u,2u,2u};
            for(unsigned i=0;i<4;++i) {
                const auto& input=bindings.attributes[locations[i]];
                textureUniforms.inputLayout[i][0]=strides[i];
                textureUniforms.inputLayout[i][1]=input.divisor;
                textureUniforms.inputLayout[i][2]=input.enabled && (i!=1 || bool(colorCache)) && (i!=2 || bool(mapCache)) && (i!=3 || bool(alphaCache));
            }
            if(deform) {textureUniforms.inputLayout[0][1]=0;textureUniforms.inputLayout[0][2]=1;}
            textureUniforms.inputLayout[1][3]=bindings.attributes[9].enabled;
            for(unsigned c=0;c<4;++c)textureUniforms.matrixEnabled[c]=bindings.attributes[5+c].enabled;
            if(instanced && !bindings.attributes[9].enabled)textureUniforms.instanceLayout[1]=0;
            for(int i=0;i<16;++i)textureUniforms.modelView[i]=float(modelView.elements()[i]);
            const auto encodedTextureUniforms = impl->encodeTextureUniforms(textureUniforms);
            draw.textureUniforms = impl->sharedBuffer(encodedTextureUniforms.bytes().size(),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, encodedTextureUniforms.bytes().data());
            draw.clippingPlanes = impl->sharedBuffer(clippingData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, clippingData.data());
            draw.descriptor = impl->textureDescriptor(draw.map, draw.alpha, draw.textureUniforms,
                draw.instanceMatrices, draw.instanceColors, draw.morphPositions, draw.morphWeights,
                draw.positions, draw.colors, draw.mapUv, draw.alphaUv, draw.clippingPlanes);
            draw.indices = indexBuffer;
            prepared.push_back(std::move(draw));
        };
        auto recordDraw = [&](const Prepared& draw) {
            if (draw.skipInstances) return;
            if(draw.invalidBindings) {
                impl->drawInfo->update(draw.logicalCount,draw.wireframe?1:4,draw.logicalInstanceCount);
                return;
            }
            impl->colorWrite = draw.colorWrite;
            impl->api.vkCmdBindPipeline(impl->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        draw.pipeline);
            const VkBuffer buffers[] = {draw.positions->buffer, draw.colors->buffer, draw.fog->buffer, draw.mapUv->buffer, draw.alphaUv->buffer};
            const VkDeviceSize offsets[] = {0, 0, 0, 0, 0};
            for (const auto& buffer : {draw.positions, draw.colors, draw.fog, draw.indices, draw.mapUv, draw.alphaUv,
                                       draw.instanceMatrices, draw.instanceColors, draw.morphPositions, draw.clippingPlanes})
                if (buffer)
                    buffer->usedEpoch = impl->recordingEpoch;
            // Vertex data is read from the prepared storage buffers using shader fetch.
            impl->api.vkCmdBindDescriptorSets(impl->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                              impl->pipelineLayout, 0, 1, &draw.descriptor, 0, nullptr);
            impl->api.vkCmdPushConstants(impl->commandBuffer, impl->pipelineLayout,
                                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                         0, sizeof(PushConstants), &draw.constants);
            if (draw.indexed) {
                impl->api.vkCmdBindIndexBuffer(impl->commandBuffer, draw.indices->buffer, 0,
                                               VK_INDEX_TYPE_UINT32);
                impl->api.vkCmdDrawIndexed(impl->commandBuffer, draw.count, draw.instanceCount, draw.start, 0, 0);
            } else
                impl->api.vkCmdDraw(impl->commandBuffer, draw.count, draw.instanceCount, draw.start, 0);
            impl->drawInfo->update(draw.logicalCount, draw.wireframe ? 1 : 4,
                                   draw.logicalInstanceCount);
        };
        auto prepareList = [&](const std::vector<Item>& list) {
            const auto overrideMaterial = scene.overrideMaterial;
            for (const auto& item : list) {
                if (!item.object->layers.test(camera.layers))
                    continue;
                const auto material = overrideMaterial && item.material->allowOverride
                                          ? overrideMaterial
                                          : item.material;
                auto* group = item.group.get();
                if (item.object->onBeforeRender)
                    item.object->onBeforeRender(*this, scene, camera, item.geometry.get(),
                                              material.get(), group);
                item.object->modelViewMatrix.multiplyMatrices(camera.matrixWorldInverse,
                                                            item.object->matrixWorld);
                item.object->normalMatrix.getNormalMatrix(item.object->modelViewMatrix);
                if (material->onBeforeRender)
                    material->onBeforeRender(*this, scene, camera, item.geometry.get(),
                                             item.object.get(), group);
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
                if (item.object->onAfterRender)
                    item.object->onAfterRender(*this, scene, camera, item.geometry.get(),
                                             material.get(), group);
            }
        };
        prepareList(opaque);
        prepareList(transparent);
        impl->endFrame();
        cleanup();
        if (scene.onAfterRender)
            scene.onAfterRender(*this, scene, camera, nullptr, nullptr, nullptr);
        impl->currentMaterialId = -1;
        impl->currentCamera = nullptr;
        impl->currentCameraId = 0;
        impl->currentCameraOwner = {};
    } catch (...) {
        impl->abortFrame();
        cleanup();
        throw;
    }
}
