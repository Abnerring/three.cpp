// Isolated real GPU oracle for original ESSL builtin semantics; not a renderer replacement.
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
    std::string name,shader;std::uint32_t height=16,first=3,baseInstance=7,instances=1;bool indexed=false,frontTest=false,front=true;float z=0.2f,nearDepth=0,farDepth=1;bool visible=true,observed=false;float sourceZ=0;bool derivatives=false;
};
struct Offscreen {
    Device& d;std::uint32_t width=20,height;VkImage image{};VkDeviceMemory imageMemory{},readMemory{},uniformMemory{},indexMemory{};
    VkImageView view{};VkRenderPass pass{};VkFramebuffer framebuffer{};VkBuffer readBuffer{},uniformBuffer{},indexBuffer{};
    VkDescriptorSetLayout descriptorLayout{};VkPipelineLayout layout{};VkDescriptorPool descriptorPool{};VkDescriptorSet descriptor{};
    VkCommandPool commandPool{};VkCommandBuffer command{};
    void allocate(const VkMemoryRequirements& r,VkMemoryPropertyFlags flags,VkDeviceMemory& memory){VkMemoryAllocateInfo a{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};a.allocationSize=r.size;a.memoryTypeIndex=d.type(r.memoryTypeBits,flags);Device::check(d.fn<PFN_vkAllocateMemory>("vkAllocateMemory")(d.device,&a,nullptr,&memory));}
    void buffer(VkDeviceSize size,VkBufferUsageFlags usage,VkBuffer& b,VkDeviceMemory& memory){VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};info.size=size;info.usage=usage;Device::check(d.fn<PFN_vkCreateBuffer>("vkCreateBuffer")(d.device,&info,nullptr,&b));VkMemoryRequirements requirements{};d.fn<PFN_vkGetBufferMemoryRequirements>("vkGetBufferMemoryRequirements")(d.device,b,&requirements);allocate(requirements,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,memory);Device::check(d.fn<PFN_vkBindBufferMemory>("vkBindBufferMemory")(d.device,b,memory,0));}
    void upload(VkDeviceMemory memory,const void* bytes,std::size_t size){void* target=nullptr;Device::check(d.fn<PFN_vkMapMemory>("vkMapMemory")(d.device,memory,0,size,0,&target));std::memcpy(target,bytes,size);d.fn<PFN_vkUnmapMemory>("vkUnmapMemory")(d.device,memory);}
    Offscreen(Device& device,std::uint32_t h):d(device),height(h){try{
        VkFormatProperties format{};d.fn<PFN_vkGetPhysicalDeviceFormatProperties>("vkGetPhysicalDeviceFormatProperties")(d.physical,VK_FORMAT_R32G32B32A32_SFLOAT,&format);if(!(format.optimalTilingFeatures&VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))throw std::runtime_error("RGBA32F color attachment unavailable");
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.imageType=VK_IMAGE_TYPE_2D;info.format=VK_FORMAT_R32G32B32A32_SFLOAT;info.extent={width,height,1};info.mipLevels=info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;info.usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        Device::check(d.fn<PFN_vkCreateImage>("vkCreateImage")(d.device,&info,nullptr,&image));VkMemoryRequirements req{};d.fn<PFN_vkGetImageMemoryRequirements>("vkGetImageMemoryRequirements")(d.device,image,&req);allocate(req,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,imageMemory);Device::check(d.fn<PFN_vkBindImageMemory>("vkBindImageMemory")(d.device,image,imageMemory,0));
        VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};vi.image=image;vi.viewType=VK_IMAGE_VIEW_TYPE_2D;vi.format=info.format;vi.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};Device::check(d.fn<PFN_vkCreateImageView>("vkCreateImageView")(d.device,&vi,nullptr,&view));
        VkAttachmentDescription attachment{};attachment.format=info.format;attachment.samples=VK_SAMPLE_COUNT_1_BIT;attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;attachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;attachment.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkAttachmentReference color{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};VkSubpassDescription sub{};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&color;
        VkSubpassDependency dependency{};dependency.srcSubpass=0;dependency.dstSubpass=VK_SUBPASS_EXTERNAL;dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;dependency.dstStageMask=VK_PIPELINE_STAGE_TRANSFER_BIT;dependency.srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;dependency.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
        VkRenderPassCreateInfo pi{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pi.attachmentCount=1;pi.pAttachments=&attachment;pi.subpassCount=1;pi.pSubpasses=&sub;pi.dependencyCount=1;pi.pDependencies=&dependency;Device::check(d.fn<PFN_vkCreateRenderPass>("vkCreateRenderPass")(d.device,&pi,nullptr,&pass));
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fi.renderPass=pass;fi.attachmentCount=1;fi.pAttachments=&view;fi.width=width;fi.height=height;fi.layers=1;Device::check(d.fn<PFN_vkCreateFramebuffer>("vkCreateFramebuffer")(d.device,&fi,nullptr,&framebuffer));
        buffer(width*height*4*sizeof(float),VK_BUFFER_USAGE_TRANSFER_DST_BIT,readBuffer,readMemory);buffer(16,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,uniformBuffer,uniformMemory);buffer(12,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,indexBuffer,indexMemory);
        const std::uint32_t indices[]{0,1,2};upload(indexMemory,indices,sizeof(indices));
        VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1,VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};li.bindingCount=1;li.pBindings=&binding;Device::check(d.fn<PFN_vkCreateDescriptorSetLayout>("vkCreateDescriptorSetLayout")(d.device,&li,nullptr,&descriptorLayout));
        VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&descriptorLayout;Device::check(d.fn<PFN_vkCreatePipelineLayout>("vkCreatePipelineLayout")(d.device,&pl,nullptr,&layout));
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,1};VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};dp.maxSets=1;dp.poolSizeCount=1;dp.pPoolSizes=&size;Device::check(d.fn<PFN_vkCreateDescriptorPool>("vkCreateDescriptorPool")(d.device,&dp,nullptr,&descriptorPool));
        VkDescriptorSetAllocateInfo ds{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};ds.descriptorPool=descriptorPool;ds.descriptorSetCount=1;ds.pSetLayouts=&descriptorLayout;Device::check(d.fn<PFN_vkAllocateDescriptorSets>("vkAllocateDescriptorSets")(d.device,&ds,&descriptor));
        VkDescriptorBufferInfo bi{uniformBuffer,0,16};VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=descriptor;write.dstBinding=0;write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;write.pBufferInfo=&bi;d.fn<PFN_vkUpdateDescriptorSets>("vkUpdateDescriptorSets")(d.device,1,&write,0,nullptr);
        VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};pool.queueFamilyIndex=d.family;Device::check(d.fn<PFN_vkCreateCommandPool>("vkCreateCommandPool")(d.device,&pool,nullptr,&commandPool));VkCommandBufferAllocateInfo cb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};cb.commandPool=commandPool;cb.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;cb.commandBufferCount=1;Device::check(d.fn<PFN_vkAllocateCommandBuffers>("vkAllocateCommandBuffers")(d.device,&cb,&command));
    }catch(...){reset();throw;}}
    VkShaderModule shader(const std::string& path){std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("SPIR-V missing "+path);auto size=std::size_t(in.tellg());if(size<20 || size%4)throw std::runtime_error("SPIR-V framing");std::vector<std::uint32_t> words(size/4);in.seekg(0);in.read(reinterpret_cast<char*>(words.data()),size);if(!in || words[0]!=0x07230203)throw std::runtime_error("SPIR-V framing");VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=size;info.pCode=words.data();VkShaderModule module{};Device::check(d.fn<PFN_vkCreateShaderModule>("vkCreateShaderModule")(d.device,&info,nullptr,&module));return module;}
    std::vector<float> draw(const Case& c,const std::string& folder){
        const struct{std::int32_t base;float framebufferHeight;float pad[2];} uniform{std::int32_t(c.baseInstance),float(height),{0,0}};upload(uniformMemory,&uniform,sizeof(uniform));
        VkShaderModule vs{},fs{};VkPipeline pipeline{};
        try{
            vs=shader(folder+"/"+c.shader+".vert.spv");fs=shader(folder+"/"+c.shader+".frag.spv");
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
int main(int argc,char** argv){try{
    if(argc!=3)throw std::runtime_error("usage: VulkanBuiltinsGPU <SPIR-V folder> <pixel-output>");
    const std::vector<Case> cases{
        {"nonindexed-first3-instance7","positive"},
        {"indexed-base6-instance11","positive",16,6,11,1,true},
        {"two-instances-base5","positive",16,3,5,2},
        {"framebuffer24-offset-viewport","positive",24,3,7},
        {"negative-GL-clip-depth","negative",16,3,7,1,false,false,true,-0.5f},
        {"GL-depth-range","negative",16,3,7,1,false,false,true,-0.5f,0.2f,0.7f},
        {"GL-CCW-front","front",16,3,7,1,false,true,true},
        {"GL-CW-back","back",16,3,7,1,false,true,false},
        {"main-early-return","early",16,3,7,1,false,false,true,0.3f,0,1,true,true,0.3f},
        {"main-intermediate-read","intermediate",16,3,7,1,false,false,true,0.2f,0,1,true,true,-0.5f},
        {"positive-w2","positivew",16,3,7},
        {"negative-w-clips","negativew",16,3,7,1,false,false,true,0.2f,0,1,false},
        {"fragment-dFdy","derivatives",16,3,7,1,false,false,true,0.2f,0,1,true,false,0,true}
    };
    Device device;std::cout<<"device="<<device.properties.deviceName<<" Vulkan="<<device.properties.apiVersion<<" deviceExtensions=0 positiveViewport=true entryWrapper=true\n";
    std::size_t checks=0,failed=0;
    for(const auto& c:cases){Offscreen target(device,c.height);const auto pixels=target.draw(c,argv[1]);
        {std::filesystem::create_directories(argv[2]);std::ofstream out(std::filesystem::path(argv[2])/(c.name+".rgba32f"),std::ios::binary);out.exceptions(std::ios::badbit|std::ios::failbit);out.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()*sizeof(float)));out.close();}
        std::size_t caseFailed=0;bool logged=false;
        for(std::uint32_t y=0;y<c.height;++y)for(std::uint32_t x=0;x<20;++x){const bool inside=c.visible && x>=3&&x<13&&y>=c.height-12&&y<c.height-4;
            float expected[4];std::fill(std::begin(expected),std::end(expected),-100.0f);
            if(inside){const float gx=x+0.5f,gy=c.height-(y+0.5f),u=(gx-3)/10,v=(gy-4)/8;const float depth=(c.z+1)*0.5f*(c.farDepth-c.nearDepth)+c.nearDepth;
                if(c.derivatives){expected[0]=1;expected[1]=1;expected[2]=1;expected[3]=0.125f;}
                else if(c.frontTest){expected[0]=c.front?1:0;expected[1]=c.front?0:1;expected[2]=depth;expected[3]=1;}
                else{expected[0]=gx;expected[1]=gy;expected[2]=depth;expected[3]=(c.instances-1)+(c.first+0.5f*u+v)/100+(c.observed?0.1f*c.sourceZ:0);}}
            for(unsigned k=0;k<4;++k){++checks;const float actual=pixels[(y*20+x)*4+k];if(!std::isfinite(actual)||std::abs(actual-expected[k])>0.0003f){++caseFailed;++failed;if(!logged){std::cout<<c.name<<" first mismatch x="<<x<<" topRow="<<y<<" channel="<<k<<" actual="<<actual<<" expected="<<expected[k]<<'\n';logged=true;}}}
        }
        std::cout<<c.name<<" failed="<<caseFailed<<"\n";
    }
    std::cout<<"cases="<<cases.size()<<" pixel-channel-checks="<<checks<<" failed="<<failed<<"\n";return failed?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 2;}}
