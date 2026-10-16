#include "catch.hpp"
#include "renderers/vulkan/VulkanPipelineLayout.h"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include "renderers/vulkan/VulkanShaderModules.h"
#include <array>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {
struct Device {
#ifdef _WIN32
    HMODULE library=nullptr;
#else
    void* library=nullptr;
#endif
    PFN_vkGetInstanceProcAddr address=nullptr;
    VkInstance instance=VK_NULL_HANDLE;
    VkDevice device=VK_NULL_HANDLE;
    PFN_vkDestroyInstance destroyInstance=nullptr;
    PFN_vkDestroyDevice destroyDevice=nullptr;
    PFN_vkCreateShaderModule createShader=nullptr;
    PFN_vkDestroyShaderModule destroyShader=nullptr;
    VulkanPipelineLayout::Dispatch dispatch;
    VkPhysicalDeviceProperties properties{};
    template<class T> T get(const char* name) {
        auto value=reinterpret_cast<T>(address(instance,name));
        if(!value)throw std::runtime_error(std::string("Vulkan function unavailable: ")+name);
        return value;
    }
    static void check(VkResult r) { if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan device setup failed: "+std::to_string(r)); }
    Device() {
        try {
#ifdef _WIN32
            library=LoadLibraryW(L"vulkan-1.dll");
            if(library)address=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(library,"vkGetInstanceProcAddr"));
#else
#ifdef __APPLE__
            for(auto name:{"libvulkan.1.dylib","libvulkan.dylib","libMoltenVK.dylib"})if(!library)library=dlopen(name,RTLD_NOW|RTLD_LOCAL);
#else
            library=dlopen("libvulkan.so.1",RTLD_NOW|RTLD_LOCAL);
#endif
            if(library)address=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(library,"vkGetInstanceProcAddr"));
#endif
            if(!address)throw std::runtime_error("Vulkan loader unavailable");
            VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_1;
            VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
            auto instanceExtensions=get<PFN_vkEnumerateInstanceExtensionProperties>("vkEnumerateInstanceExtensionProperties");
            std::uint32_t count=0;check(instanceExtensions(nullptr,&count,nullptr));
            std::vector<VkExtensionProperties> extensions(count);check(instanceExtensions(nullptr,&count,extensions.data()));
            const char* portability=VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
            for(const auto& extension:extensions)if(std::strcmp(extension.extensionName,portability)==0) {
                create.flags|=VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
                create.enabledExtensionCount=1;create.ppEnabledExtensionNames=&portability;break;
            }
            check(get<PFN_vkCreateInstance>("vkCreateInstance")(&create,nullptr,&instance));
            destroyInstance=get<PFN_vkDestroyInstance>("vkDestroyInstance");
            auto enumerate=get<PFN_vkEnumeratePhysicalDevices>("vkEnumeratePhysicalDevices");
            count=0;check(enumerate(instance,&count,nullptr));
            if(!count)throw std::runtime_error("No Vulkan physical device");
            std::vector<VkPhysicalDevice> physical(count);check(enumerate(instance,&count,physical.data()));
            auto chosen=physical.front();
            get<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties")(chosen,&properties);
            auto queues=get<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");
            queues(chosen,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);queues(chosen,&count,families.data());
            std::uint32_t family=0;
            while(family<count && !(families[family].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)))++family;
            if(family==count)throw std::runtime_error("No Vulkan graphics/compute queue");
            float priority=1;
            VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            queue.queueFamilyIndex=family;queue.queueCount=1;queue.pQueuePriorities=&priority;
            VkDeviceCreateInfo deviceCreate{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};deviceCreate.queueCreateInfoCount=1;deviceCreate.pQueueCreateInfos=&queue;
            auto deviceExtensions=get<PFN_vkEnumerateDeviceExtensionProperties>("vkEnumerateDeviceExtensionProperties");
            check(deviceExtensions(chosen,nullptr,&count,nullptr));
            extensions.resize(count);check(deviceExtensions(chosen,nullptr,&count,extensions.data()));
            const char* subset="VK_KHR_portability_subset";
            for(const auto& extension:extensions)if(std::strcmp(extension.extensionName,subset)==0) {
                deviceCreate.enabledExtensionCount=1;deviceCreate.ppEnabledExtensionNames=&subset;break;
            }
            destroyDevice=get<PFN_vkDestroyDevice>("vkDestroyDevice");
            check(get<PFN_vkCreateDevice>("vkCreateDevice")(chosen,&deviceCreate,nullptr,&device));
            dispatch={get<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout"),
                get<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout"),
                get<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout"),
                get<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")};
            createShader=get<PFN_vkCreateShaderModule>("vkCreateShaderModule");
            destroyShader=get<PFN_vkDestroyShaderModule>("vkDestroyShaderModule");
        } catch(...) { reset();throw; }
    }
    Device(const Device&)=delete;
    void reset() noexcept {
        if(device)destroyDevice(device,nullptr);device=VK_NULL_HANDLE;
        if(instance && destroyInstance)destroyInstance(instance,nullptr);instance=VK_NULL_HANDLE;
        if(library) {
#ifdef _WIN32
            FreeLibrary(library);
#else
            dlclose(library);
#endif
        }
        library=nullptr;
    }
    ~Device() { reset(); }
};
std::string read(const char* name) {
    std::ifstream stream(std::string(THREECPP_SOURCE_DIRECTORY)+name,std::ios::binary);
    if(!stream)throw std::runtime_error("Renderer shader fixture missing");
    return std::string(std::istreambuf_iterator<char>(stream),{});
}
}
TEST_CASE("Real Vulkan owns compiled shader modules with layouts and releases concurrent owners", "[vulkan][layout][gpu]") {
    VulkanShaderCompiler compiler(THREECPP_GLSLANG_EXECUTABLE);
    auto c=compiler.compile(read("/src/renderers/shaders/ShaderLib/meshbasic.vert.glsl"),
                            read("/src/renderers/shaders/ShaderLib/meshbasic.frag.glsl"));
    INFO(c.diagnostics);REQUIRE(c.linked);
    std::array stages{VulkanShaderInterface::read(c.vertex),VulkanShaderInterface::read(c.fragment)};
    Device d;INFO(d.properties.deviceName);
    std::cout<<"Vulkan layout GPU: "<<d.properties.deviceName<<"\n";
    {
        VulkanPipelineLayout layout(d.device,d.dispatch,stages,d.properties.limits,{},1);
        REQUIRE(layout.handle()!=VK_NULL_HANDLE);REQUIRE(layout.setLayouts().size()==1);
        REQUIRE(layout.description().sets[0].size()==12);
        REQUIRE(layout.description().pushConstants.size()==1);
        REQUIRE(layout.description().pushConstants[0].size==128);
        REQUIRE(layout.description().pushConstants[0].stageFlags==17);
        std::array<VulkanShaderModules::Stage,2> binaries{{{c.vertex},{c.fragment}}};
        VulkanShaderModules modules(d.device,{d.createShader,d.destroyShader},binaries);
        REQUIRE(modules.handles().size()==2);
        const auto gpuStages=modules.pipelineStages();
        REQUIRE(gpuStages[0].stage==VK_SHADER_STAGE_VERTEX_BIT);
        REQUIRE(gpuStages[1].stage==VK_SHADER_STAGE_FRAGMENT_BIT);
        REQUIRE(gpuStages[0].module!=VK_NULL_HANDLE);
        REQUIRE(gpuStages[1].module!=VK_NULL_HANDLE);

    }
    std::vector<std::future<bool>> jobs;
    for(unsigned i=0;i<12;++i)jobs.push_back(std::async(std::launch::async,[&] {
        for(unsigned n=0;n<8;++n) {
            VulkanPipelineLayout layout(d.device,d.dispatch,stages,d.properties.limits,{},1);
            std::array<VulkanShaderModules::Stage,2> binaries{{{c.vertex},{c.fragment}}};
            VulkanShaderModules modules(d.device,{d.createShader,d.destroyShader},binaries);
            if(!layout.handle() || modules.handles().size()!=2)return false;
        }
        return true;
    }));
    for(auto& job:jobs)REQUIRE(job.get());
    std::array<VulkanShaderInterface,0> none;
    VulkanPipelineLayout empty(d.device,d.dispatch,none,d.properties.limits);
    REQUIRE(empty.handle()!=VK_NULL_HANDLE);REQUIRE(empty.setLayouts().empty());
}
