#include "renderers/vulkan/VulkanPipelineLayout.h"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
using Counts = std::array<std::uint64_t, 11>;
void limit(std::uint64_t count, std::uint32_t maximum, const char* name) {
    if (count > maximum) throw std::invalid_argument(std::string("Vulkan layout exceeds ") + name);
}
void stageLimits(const Counts& c, const VkPhysicalDeviceLimits& l, std::uint32_t colorAttachments=0) {
    limit(c[0]+c[1],l.maxPerStageDescriptorSamplers,"maxPerStageDescriptorSamplers");
    limit(c[6]+c[8],l.maxPerStageDescriptorUniformBuffers,"maxPerStageDescriptorUniformBuffers");
    limit(c[7]+c[9],l.maxPerStageDescriptorStorageBuffers,"maxPerStageDescriptorStorageBuffers");
    limit(c[1]+c[2]+c[4],l.maxPerStageDescriptorSampledImages,"maxPerStageDescriptorSampledImages");
    limit(c[3]+c[5],l.maxPerStageDescriptorStorageImages,"maxPerStageDescriptorStorageImages");
    limit(c[10],l.maxPerStageDescriptorInputAttachments,"maxPerStageDescriptorInputAttachments");
    std::uint64_t resources=colorAttachments;
    for(std::size_t i=1;i<c.size();++i)resources+=c[i];
    limit(resources,l.maxPerStageResources,"maxPerStageResources");
}
void setLimits(const Counts& c, const VkPhysicalDeviceLimits& l) {
    limit(c[0]+c[1],l.maxDescriptorSetSamplers,"maxDescriptorSetSamplers");
    limit(c[6],l.maxDescriptorSetUniformBuffers,"maxDescriptorSetUniformBuffers");
    limit(c[8],l.maxDescriptorSetUniformBuffersDynamic,"maxDescriptorSetUniformBuffersDynamic");
    limit(c[7],l.maxDescriptorSetStorageBuffers,"maxDescriptorSetStorageBuffers");
    limit(c[9],l.maxDescriptorSetStorageBuffersDynamic,"maxDescriptorSetStorageBuffersDynamic");
    limit(c[1]+c[2]+c[4],l.maxDescriptorSetSampledImages,"maxDescriptorSetSampledImages");
    limit(c[3]+c[5],l.maxDescriptorSetStorageImages,"maxDescriptorSetStorageImages");
    limit(c[10],l.maxDescriptorSetInputAttachments,"maxDescriptorSetInputAttachments");
}
void checked(VkResult result,const char* operation) {
    if(result!=VK_SUCCESS)throw std::runtime_error(std::string(operation)+" failed: "+std::to_string(result));
}
}

VulkanPipelineLayout::Description VulkanPipelineLayout::describe(
    std::span<const VulkanShaderInterface> stages, const VkPhysicalDeviceLimits& limits,
    const DescriptorCounts& runtimeCounts, std::uint32_t framebufferColorAttachments) {
    Description result;
    limit(framebufferColorAttachments,limits.maxColorAttachments,"maxColorAttachments");
    std::map<std::pair<std::uint32_t,std::uint32_t>,VkDescriptorSetLayoutBinding> bindings;
    std::map<std::pair<std::uint32_t,std::uint32_t>,VkShaderStageFlags> pushes;
    VkShaderStageFlags seen=0;
    for(const auto& stage:stages) {
        // These are core graphics/compute stages. Extension stages need their
        // corresponding enabled features and extension limit records.
        if(!stage.stage || (stage.stage&(stage.stage-1)) || (stage.stage&~0x3fu))
            throw std::invalid_argument("Invalid or unsupported Vulkan shader stage");
        if(seen&stage.stage)throw std::invalid_argument("Duplicate Vulkan shader stage");
        seen|=stage.stage;
        for(const auto& d:stage.descriptors) {
            if(d.set>=limits.maxBoundDescriptorSets)
                throw std::invalid_argument("Vulkan layout exceeds maxBoundDescriptorSets");
            if(d.type>10)throw std::invalid_argument("Vulkan extension descriptor type requires extension limits");
            if(d.type==VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT && stage.stage!=VK_SHADER_STAGE_FRAGMENT_BIT)
                throw std::invalid_argument("Vulkan input attachment requires fragment stage");
            auto count=d.count;
            if(!count || std::find(d.arrayDimensions.begin(),d.arrayDimensions.end(),0)!=d.arrayDimensions.end()) {
                const auto found=runtimeCounts.find({d.set,d.binding});
                if(found==runtimeCounts.end() || !found->second)
                    throw std::invalid_argument("Runtime descriptor array requires an explicit positive capacity");
                count=found->second;
            }
            const auto key=std::pair{d.set,d.binding};
            auto [it,inserted]=bindings.emplace(key,VkDescriptorSetLayoutBinding{
                d.binding,static_cast<VkDescriptorType>(d.type),count,stage.stage,nullptr});
            if(!inserted) {
                if(it->second.descriptorType!=static_cast<VkDescriptorType>(d.type))
                    throw std::invalid_argument("Incompatible Vulkan descriptor types across stages");
                it->second.descriptorCount=std::max(it->second.descriptorCount,count);
                it->second.stageFlags|=stage.stage;
            }
        }
        if(!stage.pushConstants.empty()) {
            std::uint32_t begin=std::numeric_limits<std::uint32_t>::max(),end=0;
            for(const auto& block:stage.pushConstants) {
                // SPIRV-Reflect's block.size is the absolute end of the block;
                // block.offset is its first member, not an added base address.
                if(block.offset%4 || block.size%4 || block.size<=block.offset)
                    throw std::invalid_argument("Invalid Vulkan push constant range");
                limit(block.size,limits.maxPushConstantsSize,"maxPushConstantsSize");
                begin=std::min(begin,block.offset);end=std::max(end,block.size);
            }
            pushes[{begin,end-begin}]|=stage.stage;
        }
    }
    Counts all{};
    std::array<Counts,6> perStage{};
    for(const auto& [key,binding]:bindings) {
        if(result.sets.size()<=key.first)result.sets.resize(static_cast<std::size_t>(key.first)+1);
        result.sets[key.first].push_back(binding);
        all[binding.descriptorType]+=binding.descriptorCount;
        for(unsigned i=0;i<perStage.size();++i)
            if(binding.stageFlags&(1u<<i))perStage[i][binding.descriptorType]+=binding.descriptorCount;
    }
    setLimits(all,limits);
    for(unsigned i=0;i<perStage.size();++i)
        stageLimits(perStage[i],limits,i==4 && (seen&VK_SHADER_STAGE_FRAGMENT_BIT) ? framebufferColorAttachments : 0);
    for(const auto& [range,flags]:pushes)result.pushConstants.push_back({flags,range.first,range.second});
    return result;
}

VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device,Dispatch dispatch,
    std::span<const VulkanShaderInterface> stages,const VkPhysicalDeviceLimits& limits,
    const DescriptorCounts& runtimeCounts,std::uint32_t framebufferColorAttachments) : device_(device),dispatch_(dispatch),
    description_(describe(stages,limits,runtimeCounts,framebufferColorAttachments)) {
    if(!device || !dispatch.createSet || !dispatch.destroySet || !dispatch.createPipeline || !dispatch.destroyPipeline)
        throw std::invalid_argument("Vulkan pipeline layout requires a live device and complete dispatch");
    sets_.reserve(description_.sets.size());
    try {
        for(const auto& bindings:description_.sets) {
            VkDescriptorSetLayoutCreateInfo create{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            create.bindingCount=static_cast<std::uint32_t>(bindings.size());create.pBindings=bindings.data();
            VkDescriptorSetLayout set=VK_NULL_HANDLE;
            checked(dispatch_.createSet(device_,&create,nullptr,&set),"vkCreateDescriptorSetLayout");
            sets_.push_back(set);
        }
        VkPipelineLayoutCreateInfo create{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        create.setLayoutCount=static_cast<std::uint32_t>(sets_.size());create.pSetLayouts=sets_.data();
        create.pushConstantRangeCount=static_cast<std::uint32_t>(description_.pushConstants.size());
        create.pPushConstantRanges=description_.pushConstants.data();
        VkPipelineLayout pipeline=VK_NULL_HANDLE;
        checked(dispatch_.createPipeline(device_,&create,nullptr,&pipeline),"vkCreatePipelineLayout");
        pipeline_=pipeline;
    } catch(...) { reset();throw; }
}
VulkanPipelineLayout::~VulkanPipelineLayout() { reset(); }
VulkanPipelineLayout::VulkanPipelineLayout(VulkanPipelineLayout&& other) noexcept
    :device_(std::exchange(other.device_,VK_NULL_HANDLE)),dispatch_(other.dispatch_),
    description_(std::move(other.description_)),sets_(std::move(other.sets_)),
    pipeline_(std::exchange(other.pipeline_,VK_NULL_HANDLE)) {
    other.sets_.clear();
}
VulkanPipelineLayout& VulkanPipelineLayout::operator=(VulkanPipelineLayout&& other) noexcept {
    if(this!=&other) {
        reset();device_=std::exchange(other.device_,VK_NULL_HANDLE);dispatch_=other.dispatch_;
        description_=std::move(other.description_);sets_=std::move(other.sets_);other.sets_.clear();
        pipeline_=std::exchange(other.pipeline_,VK_NULL_HANDLE);
    }
    return *this;
}
void VulkanPipelineLayout::reset() noexcept {
    if(device_) {
        if(pipeline_)dispatch_.destroyPipeline(device_,pipeline_,nullptr);
        for(auto it=sets_.rbegin();it!=sets_.rend();++it)dispatch_.destroySet(device_,*it,nullptr);
    }
    pipeline_=VK_NULL_HANDLE;sets_.clear();device_=VK_NULL_HANDLE;
}
