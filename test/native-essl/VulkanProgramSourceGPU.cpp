// Real source program/command/uniform chain on an isolated Vulkan offscreen target.
// Device/Offscreen use the existing VulkanProgramUniformsGPU test harness.
// This does not substitute for integration in VulkanRenderer.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VK_NO_PROTOTYPES
#include <windows.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "renderers/vulkan/VulkanProgramCommands.h"
#include "renderers/webgl/WebGLPrograms.h"
#include "renderers/webgl/WebGLUniforms.h"
#include "math/Color.h"
#include <array>
#include "renderers/vulkan/VulkanProgramUniforms.h"
struct Device {
    HMODULE library=nullptr;PFN_vkGetInstanceProcAddr address=nullptr;
    VkInstance instance{};VkPhysicalDevice physical{};VkDevice device{};VkQueue queue{};
    std::uint32_t family{};VkPhysicalDeviceProperties properties{};VkPhysicalDeviceMemoryProperties memory{};
    template<class F> F fn(const char* name)const {auto p=reinterpret_cast<F>(address(instance,name));if(!p)throw std::runtime_error(std::string("Vulkan function missing: ")+name);return p;}
    static void check(VkResult r){if(r!=VK_SUCCESS)throw std::runtime_error("Vulkan error "+std::to_string(r));}
    Device(){try{
        library=LoadLibraryW(L"vulkan-1.dll");if(!library)throw std::runtime_error("Vulkan loader missing");
        address=reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(library,"vkGetInstanceProcAddr"));
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};app.apiVersion=VK_API_VERSION_1_0;
        VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
        check(fn<PFN_vkCreateInstance>("vkCreateInstance")(&create,nullptr,&instance));
        std::uint32_t count=0;auto enumerate=fn<PFN_vkEnumeratePhysicalDevices>("vkEnumeratePhysicalDevices");check(enumerate(instance,&count,nullptr));
        if(!count)throw std::runtime_error("No physical device");std::vector<VkPhysicalDevice> physicals(count);check(enumerate(instance,&count,physicals.data()));physical=physicals.front();
        fn<PFN_vkGetPhysicalDeviceProperties>("vkGetPhysicalDeviceProperties")(physical,&properties);
        fn<PFN_vkGetPhysicalDeviceMemoryProperties>("vkGetPhysicalDeviceMemoryProperties")(physical,&memory);
        auto families=fn<PFN_vkGetPhysicalDeviceQueueFamilyProperties>("vkGetPhysicalDeviceQueueFamilyProperties");families(physical,&count,nullptr);
        std::vector<VkQueueFamilyProperties> queues(count);families(physical,&count,queues.data());
        while(family<count && !(queues[family].queueFlags&VK_QUEUE_GRAPHICS_BIT))++family;if(family==count)throw std::runtime_error("No graphics queue");
        float priority=1;VkDeviceQueueCreateInfo q{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};q.queueFamilyIndex=family;q.queueCount=1;q.pQueuePriorities=&priority;
        VkDeviceCreateInfo dc{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};dc.queueCreateInfoCount=1;dc.pQueueCreateInfos=&q;
        check(fn<PFN_vkCreateDevice>("vkCreateDevice")(physical,&dc,nullptr,&device));fn<PFN_vkGetDeviceQueue>("vkGetDeviceQueue")(device,family,0,&queue);
    }catch(...){reset();throw;}}
    std::uint32_t type(std::uint32_t bits,VkMemoryPropertyFlags flags)const {for(std::uint32_t i=0;i<memory.memoryTypeCount;++i)if((bits&(1u<<i))&&(memory.memoryTypes[i].propertyFlags&flags)==flags)return i;throw std::runtime_error("Required memory type missing");}
    void reset()noexcept{if(device){fn<PFN_vkDeviceWaitIdle>("vkDeviceWaitIdle")(device);fn<PFN_vkDestroyDevice>("vkDestroyDevice")(device,nullptr);}device={};if(instance)fn<PFN_vkDestroyInstance>("vkDestroyInstance")(instance,nullptr);instance={};if(library)FreeLibrary(library);library=nullptr;}
    ~Device(){reset();}
};
struct Case {
    std::uint32_t first=0,baseInstance=0,instances=1;
    bool indexed=false;
    float nearDepth=0,farDepth=1;
};
struct Offscreen {
    Device& d;std::uint32_t width=20,height;VkImage image{};VkDeviceMemory imageMemory{},readMemory{},uniformMemory{},indexMemory{};
    VkImageView view{};VkRenderPass pass{};VkFramebuffer framebuffer{};VkBuffer readBuffer{},uniformBuffer{},indexBuffer{};
    VkDescriptorSetLayout descriptorLayout{};VkPipelineLayout layout{};VkDescriptorPool descriptorPool{};VkDescriptorSet descriptor{};
    VkCommandPool commandPool{};VkCommandBuffer command{};
    void allocate(const VkMemoryRequirements& r,VkMemoryPropertyFlags flags,VkDeviceMemory& memory){VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=d.type(r.memoryTypeBits,flags);Device::check(d.fn<PFN_vkAllocateMemory>("vkAllocateMemory")(d.device,&a,nullptr,&memory));}
    void buffer(VkDeviceSize size,VkBufferUsageFlags usage,VkBuffer& b,VkDeviceMemory& memory){VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=usage;Device::check(d.fn<PFN_vkCreateBuffer>("vkCreateBuffer")(d.device,&info,nullptr,&b));VkMemoryRequirements requirements{};d.fn<PFN_vkGetBufferMemoryRequirements>("vkGetBufferMemoryRequirements")(d.device,b,&requirements);allocate(requirements,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,memory);Device::check(d.fn<PFN_vkBindBufferMemory>("vkBindBufferMemory")(d.device,b,memory,0));}
    void upload(VkDeviceMemory memory,const void* bytes,std::size_t size){void* target=nullptr;Device::check(d.fn<PFN_vkMapMemory>("vkMapMemory")(d.device,memory,0,size,0,&target));std::memcpy(target,bytes,size);d.fn<PFN_vkUnmapMemory>("vkUnmapMemory")(d.device,memory);}
    Offscreen(Device& device,std::uint32_t h,std::size_t uniformSize):d(device),height(h){try{
        VkFormatProperties format{};d.fn<PFN_vkGetPhysicalDeviceFormatProperties>("vkGetPhysicalDeviceFormatProperties")(d.physical,VK_FORMAT_R32G32B32A32_SFLOAT,&format);if(!(format.optimalTilingFeatures&VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))throw std::runtime_error("RGBA32F color attachment unavailable");
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;info.format=VK_FORMAT_R32G32B32A32_SFLOAT;info.extent={width,height,1};info.mipLevels=info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        Device::check(d.fn<PFN_vkCreateImage>("vkCreateImage")(d.device,&info,nullptr,&image));VkMemoryRequirements req{};d.fn<PFN_vkGetImageMemoryRequirements>("vkGetImageMemoryRequirements")(d.device,image,&req);allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,imageMemory);Device::check(d.fn<PFN_vkBindImageMemory>("vkBindImageMemory")(d.device,image,imageMemory,0));
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=info.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};Device::check(d.fn<PFN_vkCreateImageView>("vkCreateImageView")(d.device,&vi,nullptr,&view));
        VkAttachmentDescription attachment{};attachment.format=info.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;
        VkSubpassDependency dependency{};dependency.srcSubpass=0;dependency.dstSubpass=VK_SUBPASS_EXTERNAL;dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dependency.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;dependency.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;dependency.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        VkRenderPassCreateInfo pi{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pi.attachmentCount=1;pi.pAttachments=&attachment;pi.subpassCount=1;pi.pSubpasses=&sub;pi.dependencyCount=1;pi.pDependencies=&dependency;Device::check(d.fn<PFN_vkCreateRenderPass>("vkCreateRenderPass")(d.device,&pi,nullptr,&pass));
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fi.renderPass=pass;fi.attachmentCount=1;fi.pAttachments=&view;fi.width=width;fi.height=height;fi.layers=1;Device::check(d.fn<PFN_vkCreateFramebuffer>("vkCreateFramebuffer")(d.device,&fi,nullptr,&framebuffer));
        buffer(width*height*4*sizeof(float),VK_BUFFER_USAGE_TRANSFER_DST_BIT,readBuffer,readMemory);buffer(uniformSize,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,uniformBuffer,uniformMemory);buffer(12,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indexBuffer,indexMemory);
        const std::uint32_t indices[]{0,1,2};upload(indexMemory,indices,sizeof(indices));
        VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};li.bindingCount=1;li.pBindings=&binding;Device::check(d.fn<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout")(d.device,&li,nullptr,&descriptorLayout));
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&descriptorLayout;Device::check(d.fn<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout")(d.device,&pl,nullptr,&layout));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1};VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=1;dp.pPoolSizes=&size;Device::check(d.fn<PFN_vkCreateDescriptorPool>("vkCreateDescriptorPool")(d.device,&dp,nullptr,&descriptorPool));
        VkDescriptorSetAllocateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ds.descriptorPool=descriptorPool;ds.descriptorSetCount=1;ds.pSetLayouts=&descriptorLayout;Device::check(d.fn<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets")(d.device,&ds,&descriptor));
        VkDescriptorBufferInfo bi{uniformBuffer,0,uniformSize};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=0;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&bi;d.fn<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(d.device,1,&write,0,nullptr);
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=d.family;Device::check(d.fn<PFN_vkCreateCommandPool>("vkCreateCommandPool")(d.device,&pool,nullptr,&commandPool));VkCommandBufferAllocateInfo cb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};cb.commandPool=commandPool;cb.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cb.commandBufferCount=1;Device::check(d.fn<PFN_vkAllocateCommandBuffers>("vkAllocateCommandBuffers")(d.device,&cb,&command));
    }catch(...){reset();throw;}}
    VkShaderModule shader(std::span<const std::uint32_t> words){if(words.size()<5 || words[0]!=0x07230203)throw std::runtime_error("SPIR-V framing");VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=words.size_bytes();info.pCode=words.data();VkShaderModule module{};Device::check(d.fn<PFN_vkCreateShaderModule>("vkCreateShaderModule")(d.device,&info,nullptr,&module));return module;}
    std::vector<float> draw(const Case& c,const VulkanEsslResult& program,VulkanProgramUniforms& uniforms){
        uniforms.writeBackend(c.baseInstance,float(height));const auto data=uniforms.bytes({0,0});upload(uniformMemory,data.data(),data.size());
        VkShaderModule vs{},fs{};VkPipeline pipeline{};
        try{
            vs=shader(program.modules[0].words);fs=shader(program.modules[1].words);
            VkPipelineShaderStageCreateInfo shaders[2]{{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO},{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}};shaders[0].stage=VK_SHADER_STAGE_VERTEX_BIT;shaders[0].module=vs;shaders[0].pName="main";shaders[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;shaders[1].module=fs;shaders[1].pName="main";
            VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkViewport viewport{3,float(height)-12,10,8,c.nearDepth,c.farDepth};VkRect2D scissor{{3,int(height)-12},{10,8}};
            VkPipelineViewportStateCreateInfo viewportInfo{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewportInfo.viewportCount=1;viewportInfo.pViewports=&viewport;viewportInfo.scissorCount=1;viewportInfo.pScissors=&scissor;
            VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;raster.lineWidth=1;
            VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
            VkPipelineColorBlendAttachmentState attachment{};attachment.colorWriteMask=15;VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};blend.attachmentCount=1;blend.pAttachments=&attachment;
            VkGraphicsPipelineCreateInfo p{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};p.stageCount=2;p.pStages=shaders;p.pVertexInputState=&vertex;p.pInputAssemblyState=&assembly;p.pViewportState=&viewportInfo;p.pRasterizationState=&raster;p.pMultisampleState=&samples;p.pColorBlendState=&blend;p.layout=layout;p.renderPass=pass;
            Device::check(d.fn<PFN_vkCreateGraphicsPipelines>("vkCreateGraphicsPipelines")(d.device,VK_NULL_HANDLE,1,&p,nullptr,&pipeline));
            Device::check(d.fn<PFN_vkResetCommandPool>("vkResetCommandPool")(d.device,commandPool,0));VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};Device::check(d.fn<PFN_vkBeginCommandBuffer>("vkBeginCommandBuffer")(command,&begin));
            VkClearValue clear{};for(float& x:clear.color.float32)x=-100;
            VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};render.renderPass=pass;render.framebuffer=framebuffer;render.renderArea={{0,0},{width,height}};render.clearValueCount=1;render.pClearValues=&clear;
            d.fn<PFN_vkCmdBeginRenderPass>("vkCmdBeginRenderPass")(command,&render,VK_SUBPASS_CONTENTS_INLINE);d.fn<PFN_vkCmdBindPipeline>("vkCmdBindPipeline")(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);d.fn<PFN_vkCmdBindDescriptorSets>("vkCmdBindDescriptorSets")(command,VK_PIPELINE_BIND_POINT_GRAPHICS,layout,0,1,&descriptor,0,nullptr);
            if(c.indexed){d.fn<PFN_vkCmdBindIndexBuffer>("vkCmdBindIndexBuffer")(command,indexBuffer,0,VK_INDEX_TYPE_UINT32);d.fn<PFN_vkCmdDrawIndexed>("vkCmdDrawIndexed")(command,3,c.instances,0,std::int32_t(c.first),c.baseInstance);}else d.fn<PFN_vkCmdDraw>("vkCmdDraw")(command,3,c.instances,c.first,c.baseInstance);
            d.fn<PFN_vkCmdEndRenderPass>("vkCmdEndRenderPass")(command);
            VkBufferImageCopy copy{};copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};copy.imageExtent={width,height,1};d.fn<PFN_vkCmdCopyImageToBuffer>("vkCmdCopyImageToBuffer")(command,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,readBuffer,1,&copy);
            VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.buffer=readBuffer;barrier.size=VK_WHOLE_SIZE;d.fn<PFN_vkCmdPipelineBarrier>("vkCmdPipelineBarrier")(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,nullptr,1,&barrier,0,nullptr);
            Device::check(d.fn<PFN_vkEndCommandBuffer>("vkEndCommandBuffer")(command));VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.commandBufferCount=1;submit.pCommandBuffers=&command;Device::check(d.fn<PFN_vkQueueSubmit>("vkQueueSubmit")(d.queue,1,&submit,VK_NULL_HANDLE));Device::check(d.fn<PFN_vkQueueWaitIdle>("vkQueueWaitIdle")(d.queue));
            std::vector<float> pixels(width*height*4);void* bytes=nullptr;Device::check(d.fn<PFN_vkMapMemory>("vkMapMemory")(d.device,readMemory,0,pixels.size()*sizeof(float),0,&bytes));std::memcpy(pixels.data(),bytes,pixels.size()*sizeof(float));d.fn<PFN_vkUnmapMemory>("vkUnmapMemory")(d.device,readMemory);
            d.fn<PFN_vkDestroyPipeline>("vkDestroyPipeline")(d.device,pipeline,nullptr);d.fn<PFN_vkDestroyShaderModule>("vkDestroyShaderModule")(d.device,fs,nullptr);d.fn<PFN_vkDestroyShaderModule>("vkDestroyShaderModule")(d.device,vs,nullptr);return pixels;
        }catch(...){if(pipeline)d.fn<PFN_vkDestroyPipeline>("vkDestroyPipeline")(d.device,pipeline,nullptr);if(fs)d.fn<PFN_vkDestroyShaderModule>("vkDestroyShaderModule")(d.device,fs,nullptr);if(vs)d.fn<PFN_vkDestroyShaderModule>("vkDestroyShaderModule")(d.device,vs,nullptr);throw;}
    }
    void reset()noexcept{
        if(commandPool)d.fn<PFN_vkDestroyCommandPool>("vkDestroyCommandPool")(d.device,commandPool,nullptr);
        if(descriptorPool)d.fn<PFN_vkDestroyDescriptorPool>("vkDestroyDescriptorPool")(d.device,descriptorPool,nullptr);
        if(layout)d.fn<PFN_vkDestroyPipelineLayout>("vkDestroyPipelineLayout")(d.device,layout,nullptr);
        if(descriptorLayout)d.fn<PFN_vkDestroyDescriptorSetLayout>("vkDestroyDescriptorSetLayout")(d.device,descriptorLayout,nullptr);
        if(framebuffer)d.fn<PFN_vkDestroyFramebuffer>("vkDestroyFramebuffer")(d.device,framebuffer,nullptr);
        if(pass)d.fn<PFN_vkDestroyRenderPass>("vkDestroyRenderPass")(d.device,pass,nullptr);
        if(view)d.fn<PFN_vkDestroyImageView>("vkDestroyImageView")(d.device,view,nullptr);
        if(image)d.fn<PFN_vkDestroyImage>("vkDestroyImage")(d.device,image,nullptr);
        for(auto b:{readBuffer,uniformBuffer,indexBuffer})if(b)d.fn<PFN_vkDestroyBuffer>("vkDestroyBuffer")(d.device,b,nullptr);
        for(auto m:{imageMemory,readMemory,uniformMemory,indexMemory})if(m)d.fn<PFN_vkFreeMemory>("vkFreeMemory")(d.device,m,nullptr);
    }
    ~Offscreen(){reset();}
};

namespace {
using Args=std::vector<JSValue>;
void require(bool condition,const std::string& message) {
    if(!condition)throw std::runtime_error(message);
}
JSValue call(JSValue object,const char* method,const Args& args={}) {
    return object.get(method).call(object,args);
}
constexpr auto vertexSource=R"(precision highp float;
void main() {
    vec2 vertices[3]=vec2[3](vec2(-1.,-1.),vec2(3.,-1.),vec2(-1.,3.));
    gl_Position=vec4(vertices[gl_VertexID],0.,1.);
})";
constexpr auto fragmentSource=R"(precision highp float;
uniform vec3 diffuse;
uniform float opacity;
out vec4 result;
void main() { result=vec4(diffuse,opacity); }
)";
constexpr auto changedFragmentSource=R"(precision highp float;
uniform vec3 diffuse;
uniform float opacity;
out vec4 result;
void main() { result=vec4(vec3(1.)-diffuse,1.-opacity); }
)";

struct SourcePrograms {
    VulkanProgramCommands commands;
    JSValue gl=commands.object(),renderer=JSValue::object(),bindings=JSValue::object();
    JSValue counters=JSValue::object();
    WebGLPrograms cache;
    SourcePrograms():cache(makeRenderer(),{},{},JSValue::object(),bindings,{}) {}
    JSValue makeRenderer() {
        counters.set("releases",0);
        renderer.set("getContext",JSValue::function(
            [](const JSValue&,const Args&,const Args& captured){return captured[0];},{gl}));
        auto debug=JSValue::object();debug.set("checkShaderErrors",false);renderer.set("debug",debug);
        bindings.set("releaseStatesOfProgram",JSValue::function(
            [](const JSValue&,const Args&,const Args& captured){
                auto state=captured[0];state.set("releases",state.get("releases").number()+1);return JSValue();
            },{counters}));
        return renderer;
    }
    JSValue parameters(bool changed=false) {
        auto p=JSValue::object();
        for(const char* name:{"numSunLights","numDirLights","numSpotLights","numSpotLightMaps",
            "numSpotLightShadows","numSpotLightShadowsWithMaps","numRectAreaLights","numPointLights",
            "numHemiLights","numSunLightShadows","numDirLightShadows","numPointLightShadows",
            "numClippingPlanes","numClipIntersection"})p.set(name,0);
        p.set("precision","highp");p.set("shaderType","RawShaderMaterial");
        p.set("shaderName","source-program-gpu");p.set("glslVersion","300 es");
        p.set("vertexShader",vertexSource);p.set("fragmentShader",changed?changedFragmentSource:fragmentSource);
        p.set("toneMapping",0);p.set("outputColorSpace","srgb");p.set("envMap",false);
        p.set("isRawShaderMaterial",true);p.set("rendererExtensionParallelShaderCompile",false);
        p.set("defines",JSValue::object());
        return p;
    }
    std::shared_ptr<VulkanProgramCommands::LinkedProgram> linked(const JSValue& program) {
        return linkedHandle(program.get("program"));
    }
    std::shared_ptr<VulkanProgramCommands::LinkedProgram> linkedHandle(const JSValue& handle) {
        require(call(gl,"getProgramParameter",{handle,gl.get("LINK_STATUS")}).strictEquals(true),
            "Source WebGLProgram failed to link: "+call(gl,"getProgramInfoLog",{handle}).toString());
        auto result=commands.linked(handle);
        require(result!=nullptr,"Linked source program was not owned by native commands");
        for(const auto& module:result->result.modules)
            require(module.validated&&!module.words.empty(),"Source program has no validated native SPIR-V");
        // This fixture deliberately has only original numeric uniforms. Verify
        // the real reflection before using the single-UBO low-level test target.
        const auto& blocks=result->uniforms->blocks();
        require(blocks.size()==1&&blocks.begin()->first.set==0&&blocks.begin()->first.binding==0,
            "Unexpected source program uniform block interface");
        require(result->uniforms->samplers().empty(),"Unexpected source program samplers");
        require(result->activeAttributes.empty(),"Fullscreen source unexpectedly needs vertex inputs");
        return result;
    }
};

struct PixelChecks {
    Device& device;
    SourcePrograms& source;
    std::filesystem::path output;
    std::size_t cases=0,channels=0;
    void draw(const char* name,const JSValue& program,const std::array<float,4>& expected) {
        drawHandle(name,program.get("program"),expected);
    }
    void drawHandle(const char* name,const JSValue& handle,const std::array<float,4>& expected) {
        const auto linked=source.linkedHandle(handle);
        require(source.commands.current()==linked,"GPU test must execute the actual current native program");
        Offscreen target(device,16,linked->uniforms->bytes({0,0}).size());
        const auto pixels=target.draw(Case{},linked->result,*linked->uniforms);
        if(!output.empty()){
            std::filesystem::create_directories(output);
            std::ofstream file(output/(std::string(name)+".rgba32f"),std::ios::binary);
            file.exceptions(std::ios::badbit|std::ios::failbit);
            file.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*sizeof(float)));
            file.close();
        }
        for(std::uint32_t y=0;y<target.height;++y)for(std::uint32_t x=0;x<target.width;++x){
            const bool inside=x>=3&&x<13&&y>=target.height-12&&y<target.height-4;
            for(unsigned channel=0;channel<4;++channel){
                ++channels;
                const float wanted=inside?expected[channel]:-100.f;
                const auto actual=pixels[(y*target.width+x)*4+channel];
                require(std::isfinite(actual)&&std::abs(actual-wanted)<0.00001f,
                    std::string(name)+" x="+std::to_string(x)+" topRow="+std::to_string(y)+
                    " channel="+std::to_string(channel)+" actual="+std::to_string(actual)+
                    " expected="+std::to_string(wanted));
            }
        }
        require(call(source.gl,"getError").strictEquals(0),std::string(name)+" native command error");
        ++cases;std::cout<<name<<" passed\n";
    }
};
}

int main(int argc,char** argv) {
    try {
        require(argc<=2,"usage: VulkanProgramSourceGPU [pixel-output-directory]");
        Device device;SourcePrograms source;
        PixelChecks pixels{device,source,argc==2?std::filesystem::path(argv[1]):std::filesystem::path{}};
        std::cout<<"device="<<device.properties.deviceName
                 <<" sourcePrograms=true nativeESSL=true validationRequested=false validationEnabled=false\n";
        auto program=source.cache.acquireProgram(source.parameters(),"shared");
        const auto handle=program.get("program");
        const auto first=source.linked(program);
        require(call(source.gl,"getShaderSource",{program.get("vertexShader")}).toString().find(vertexSource)!=std::string::npos,
            "Original vertex shader body was changed before native compilation");
        require(call(source.gl,"getShaderSource",{program.get("fragmentShader")}).toString().find(fragmentSource)!=std::string::npos,
            "Original fragment shader body was changed before native compilation");
        auto reflected=call(program,"getUniforms");
        require(reflected.sameValue(call(program,"getUniforms")),"Original uniform reflection cache identity changed");
        auto uniforms=WebGLUniforms::fromObject(reflected);
        auto color=nativeColorConstructor().construct({.25,.5,.75});
        call(source.gl,"useProgram",{handle});
        uniforms.setValue(source.gl,"diffuse",color);uniforms.setValue(source.gl,"opacity",.5);
        pixels.draw("initial-native-upload",program,{.25f,.5f,.75f,.5f});

        color.set("r",.875);
        pixels.draw("color-mutation-without-upload",program,{.25f,.5f,.75f,.5f});
        uniforms.setValue(source.gl,"diffuse",color);
        pixels.draw("source-color-component-refresh",program,{.875f,.5f,.75f,.5f});
        auto revision=first->uniformRevision;
        uniforms.setValue(source.gl,"diffuse",color);uniforms.setValue(source.gl,"opacity",.5);
        require(first->uniformRevision==revision,"Unchanged source uniform values unexpectedly uploaded");
        pixels.draw("unchanged-source-cache",program,{.875f,.5f,.75f,.5f});

        const auto opacity=call(source.gl,"getUniformLocation",{handle,"opacity"});
        call(source.gl,"uniform1f",{opacity,.125});revision=first->uniformRevision;
        uniforms.setValue(source.gl,"opacity",.5);
        require(first->uniformRevision==revision,"Source uniform cache was bypassed after a native command");
        pixels.draw("source-cache-retains-native-command",program,{.875f,.5f,.75f,.125f});
        uniforms.setValue(source.gl,"opacity",.625);
        pixels.draw("source-cache-value-change",program,{.875f,.5f,.75f,.625f});

        const auto shared=source.cache.acquireProgram(source.parameters(true),"shared");
        require(shared.sameValue(program)&&source.commands.linked(shared.get("program"))==first,
            "Original shared cache key must retain the original actual native program");
        require(program.get("usedTimes").strictEquals(2),"Original shared program reference count mismatch");
        pixels.draw("shared-key-retains-original-shader",shared,{.875f,.5f,.75f,.625f});

        auto changed=source.cache.acquireProgram(source.parameters(true),"changed");
        const auto second=source.linked(changed);
        require(second!=first&&second->result.modules[1].words!=first->result.modules[1].words,
            "Different source shader key did not compile different real fragment SPIR-V");
        call(source.gl,"useProgram",{changed.get("program")});
        auto changedUniforms=WebGLUniforms::fromObject(call(changed,"getUniforms"));
        changedUniforms.setValue(source.gl,"diffuse",color);changedUniforms.setValue(source.gl,"opacity",.625);
        pixels.draw("changed-source-shader",changed,{.125f,.5f,.25f,.375f});

        call(source.gl,"useProgram",{handle});
        uniforms.setValue(source.gl,"diffuse",color);uniforms.setValue(source.gl,"opacity",.625);
        pixels.draw("switch-back-retains-original-program-bytes",program,{.875f,.5f,.75f,.625f});
        const auto otherLocation=call(source.gl,"getUniformLocation",{changed.get("program"),"opacity"});
        call(source.gl,"uniform1f",{otherLocation,0});
        require(call(source.gl,"getError").strictEquals(source.gl.get("INVALID_OPERATION")),
            "Foreign current-program uniform location was accepted");
        pixels.draw("foreign-location-retains-original-bytes",program,{.875f,.5f,.75f,.625f});

        source.cache.releaseProgram(program);
        require(program.get("program").sameValue(handle)&&source.counters.get("releases").strictEquals(0),
            "First shared reference release prematurely destroyed the native source program");
        pixels.draw("one-shared-reference-remains",program,{.875f,.5f,.75f,.625f});
        source.cache.releaseProgram(shared);
        require(program.get("program").isUndefined(),"Source program wrapper retained its released GL handle");
        require(call(source.gl,"getProgramParameter",{handle,source.gl.get("DELETE_STATUS")}).strictEquals(true),
            "Final source cache release did not mark the actual current native program for deletion");
        require(source.commands.currentProgram().sameValue(handle)&&source.commands.linked(handle)==first,
            "Deleting the current program prematurely discarded its compiled executable");
        pixels.drawHandle("deleted-current-retains-native-executable",handle,{.875f,.5f,.75f,.625f});
        require(source.counters.get("releases").strictEquals(1),"Source binding release count mismatch");
        auto recreated=source.cache.acquireProgram(source.parameters(true),"shared");
        require(!recreated.sameValue(program),"Released source cache identity survived recreation");
        const auto recreatedHandle=recreated.get("program");
        call(source.gl,"useProgram",{recreatedHandle});
        require(source.commands.linked(handle)==nullptr,
            "Switching current programs did not finally release the deleted native executable");
        auto recreatedUniforms=WebGLUniforms::fromObject(call(recreated,"getUniforms"));
        recreatedUniforms.setValue(source.gl,"diffuse",color);recreatedUniforms.setValue(source.gl,"opacity",.625);
        pixels.draw("released-key-recompiles-changed-source",recreated,{.125f,.5f,.25f,.375f});
        source.cache.releaseProgram(changed);source.cache.releaseProgram(recreated);
        require(source.cache.object().get("programs").length()==0&&source.counters.get("releases").strictEquals(3),
            "Source program cache did not remove all released program wrappers");
        require(call(source.gl,"getProgramParameter",{recreatedHandle,source.gl.get("DELETE_STATUS")}).strictEquals(true),
            "Final current source program was not marked for deletion");
        call(source.gl,"useProgram",{nullptr});
        require(source.commands.currentProgram().isNull()&&source.commands.linked(recreatedHandle)==nullptr,
            "Clearing the current program did not finish its deferred native release");
        require(call(source.gl,"getError").strictEquals(0),"Final native program command error");
        std::cout<<"cases="<<pixels.cases<<" pixel-channel-checks="<<pixels.channels<<" failed=0\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 2;}
}
