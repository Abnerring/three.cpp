#include "catch.hpp"
#include "renderers/vulkan/VulkanPipelineLayout.h"
#include "renderers/vulkan/VulkanShaderCompiler.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace {
VkPhysicalDeviceLimits limits() {
    VkPhysicalDeviceLimits l{};
    l.maxBoundDescriptorSets=8;l.maxPushConstantsSize=128;l.maxColorAttachments=8;
    l.maxPerStageDescriptorSamplers=l.maxPerStageDescriptorUniformBuffers=1024;
    l.maxPerStageDescriptorStorageBuffers=l.maxPerStageDescriptorSampledImages=1024;
    l.maxPerStageDescriptorStorageImages=l.maxPerStageDescriptorInputAttachments=1024;
    l.maxPerStageResources=1024;
    l.maxDescriptorSetSamplers=l.maxDescriptorSetUniformBuffers=1024;
    l.maxDescriptorSetUniformBuffersDynamic=l.maxDescriptorSetStorageBuffers=1024;
    l.maxDescriptorSetStorageBuffersDynamic=l.maxDescriptorSetSampledImages=1024;
    l.maxDescriptorSetStorageImages=l.maxDescriptorSetInputAttachments=1024;
    return l;
}
VulkanShaderInterface shader(unsigned stage,unsigned set,unsigned binding,unsigned type,unsigned count) {
    VulkanShaderInterface s;s.stage=stage;
    VulkanShaderInterface::Descriptor d;d.set=set;d.binding=binding;d.type=type;d.count=count;
    s.descriptors.push_back(d);return s;
}
struct Tracker {
    unsigned attempts=0,failSet=0;bool failPipeline=false;
    std::vector<std::uintptr_t> live,destroyed;
    bool pipelineLive=false;
};
thread_local Tracker* tracker=nullptr;
VKAPI_ATTR VkResult VKAPI_CALL createSet(VkDevice,const VkDescriptorSetLayoutCreateInfo*,const VkAllocationCallbacks*,VkDescriptorSetLayout* result) {
    ++tracker->attempts;
    if(tracker->attempts==tracker->failSet){*result=reinterpret_cast<VkDescriptorSetLayout>(999);return VK_ERROR_OUT_OF_DEVICE_MEMORY;}
    const auto id=tracker->attempts;
    *result=reinterpret_cast<VkDescriptorSetLayout>(static_cast<std::uintptr_t>(id));tracker->live.push_back(id);return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL destroySet(VkDevice,VkDescriptorSetLayout handle,const VkAllocationCallbacks*) {
    const auto id=reinterpret_cast<std::uintptr_t>(handle);
    tracker->destroyed.push_back(id);
    auto it=std::find(tracker->live.begin(),tracker->live.end(),id);
    if(it==tracker->live.end())throw std::runtime_error("Double destroy or failed creation handle destroyed");
    tracker->live.erase(it);
}
VKAPI_ATTR VkResult VKAPI_CALL createPipeline(VkDevice,const VkPipelineLayoutCreateInfo*,const VkAllocationCallbacks*,VkPipelineLayout* result) {
    if(tracker->failPipeline){*result=reinterpret_cast<VkPipelineLayout>(999);return VK_ERROR_OUT_OF_HOST_MEMORY;}
    *result=reinterpret_cast<VkPipelineLayout>(100);tracker->pipelineLive=true;return VK_SUCCESS;
}
VKAPI_ATTR void VKAPI_CALL destroyPipeline(VkDevice,VkPipelineLayout,const VkAllocationCallbacks*) {
    if(!tracker->pipelineLive)throw std::runtime_error("Failed or already destroyed pipeline");
    tracker->pipelineLive=false;
}
const VulkanPipelineLayout::Dispatch dispatch{createSet,destroySet,createPipeline,destroyPipeline};
const auto device=reinterpret_cast<VkDevice>(1);
}
TEST_CASE("Pipeline layout merges shared descriptors and preserves sparse sets", "[vulkan][layout]") {
    std::array stages{shader(1,2,4,6,1),shader(16,2,4,6,1)};
    stages[1].descriptors.push_back(shader(16,1,7,1,3).descriptors[0]);
    auto d=VulkanPipelineLayout::describe(stages,limits());
    REQUIRE(d.sets.size()==3);REQUIRE(d.sets[0].empty());REQUIRE(d.sets[1].size()==1);
    REQUIRE(d.sets[1][0].binding==7);REQUIRE(d.sets[1][0].descriptorCount==3);
    REQUIRE(d.sets[2][0].binding==4);REQUIRE(d.sets[2][0].stageFlags==17);
    REQUIRE(d.sets[2][0].pImmutableSamplers==nullptr);
    stages[1].descriptors[0].count=5;
    d=VulkanPipelineLayout::describe(stages,limits());REQUIRE(d.sets[2][0].descriptorCount==5);
    stages[1].descriptors[0].type=7;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits()),std::invalid_argument);
}
TEST_CASE("Real reflection handles nonzero push constant offsets without doubling the base", "[vulkan][layout]") {
    VulkanShaderCompiler compiler(THREECPP_GLSLANG_EXECUTABLE);
    auto c=compiler.compile(R"(#version 450
layout(push_constant) uniform Push { layout(offset=32) vec4 offset; } pc;
void main(){gl_Position=pc.offset;}
)",R"(#version 450
layout(location=0) out vec4 color;
void main(){color=vec4(1);}
)");INFO(c.diagnostics);REQUIRE(c.linked);
    std::array stages{VulkanShaderInterface::read(c.vertex),VulkanShaderInterface::read(c.fragment)};
    REQUIRE(stages[0].pushConstants[0].offset==32);REQUIRE(stages[0].pushConstants[0].size==48);
    auto d=VulkanPipelineLayout::describe(stages,limits());REQUIRE(d.pushConstants.size()==1);
    REQUIRE(d.pushConstants[0].offset==32);REQUIRE(d.pushConstants[0].size==16);
    REQUIRE(d.pushConstants[0].stageFlags==1);
    stages[1].pushConstants=stages[0].pushConstants;
    d=VulkanPipelineLayout::describe(stages,limits());REQUIRE(d.pushConstants.size()==1);REQUIRE(d.pushConstants[0].stageFlags==17);
    stages[1].pushConstants[0].offset=16;stages[1].pushConstants[0].size=64;
    d=VulkanPipelineLayout::describe(stages,limits());REQUIRE(d.pushConstants.size()==2);
    REQUIRE((d.pushConstants[0].stageFlags&d.pushConstants[1].stageFlags)==0);
}
TEST_CASE("Layout validates stage conflicts capacities and device limits before GPU calls", "[vulkan][layout]") {
    std::array stages{shader(1,0,0,1,3),shader(16,0,0,1,5)};
    auto l=limits();l.maxPerStageDescriptorSamplers=4;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,l),std::invalid_argument);
    l=limits();l.maxDescriptorSetSamplers=4;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,l),std::invalid_argument);
    l=limits();l.maxPerStageResources=4;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,l),std::invalid_argument);
    l=limits();l.maxPerStageResources=5;
    REQUIRE_NOTHROW(VulkanPipelineLayout::describe(stages,l));
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,l,{},1),std::invalid_argument);
    l.maxPerStageResources=6;
    REQUIRE_NOTHROW(VulkanPipelineLayout::describe(stages,l,{},1));
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,l,{},9),std::invalid_argument);
    stages[1].stage=1;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits()),std::invalid_argument);
    stages[1].stage=0;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits()),std::invalid_argument);
    stages[1].stage=3;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits()),std::invalid_argument);
    stages[1].stage=16;stages[1].descriptors[0].set=8;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits()),std::invalid_argument);
    stages[1].descriptors[0].set=0;stages[1].descriptors[0].count=0;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits()),std::invalid_argument);
    auto d=VulkanPipelineLayout::describe(stages,limits(),{{{0,0},8}});
    REQUIRE(d.sets[0][0].descriptorCount==8);
    stages[0].descriptors[0].type=10;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits(),{{{0,0},8}}),std::invalid_argument);
    stages[0].descriptors[0].type=6;stages[1].descriptors[0].type=6;
    VulkanShaderInterface::Block block;block.offset=2;block.size=16;stages[0].pushConstants={block};
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits(),{{{0,0},8}}),std::invalid_argument);
    stages[0].pushConstants[0].offset=0;stages[0].pushConstants[0].size=132;
    REQUIRE_THROWS_AS(VulkanPipelineLayout::describe(stages,limits(),{{{0,0},8}}),std::invalid_argument);
}
TEST_CASE("Failed layout creation rolls back only successful handles and move transfers ownership", "[vulkan][layout]") {
    std::array stages{shader(1,2,4,6,1)};
    for(unsigned fail:{1u,2u,3u,0u}) {
        Tracker t;t.failSet=fail;t.failPipeline=!fail;tracker=&t;
        REQUIRE_THROWS_AS(VulkanPipelineLayout(device,dispatch,stages,limits()),std::runtime_error);
        REQUIRE(t.live.empty());REQUIRE_FALSE(t.pipelineLive);
        REQUIRE(t.destroyed.size()==(fail?fail-1:3));
        REQUIRE(std::is_sorted(t.destroyed.rbegin(),t.destroyed.rend()));
    }
    Tracker t;tracker=&t;
    {
        VulkanPipelineLayout first(device,dispatch,stages,limits());REQUIRE(t.live.size()==3);REQUIRE(t.pipelineLive);
        auto handle=first.handle();VulkanPipelineLayout second(std::move(first));
        REQUIRE(first.handle()==VK_NULL_HANDLE);REQUIRE(first.setLayouts().empty());REQUIRE(second.handle()==handle);
        second=std::move(second);REQUIRE(second.handle()==handle);
        first=std::move(second);REQUIRE(first.handle()==handle);REQUIRE(second.handle()==VK_NULL_HANDLE);
        first.reset();first.reset();REQUIRE(t.live.empty());REQUIRE_FALSE(t.pipelineLive);
    }
    REQUIRE(t.destroyed==std::vector<std::uintptr_t>{3,2,1});
    Tracker invalid;tracker=&invalid;
    REQUIRE_THROWS_AS(VulkanPipelineLayout(VK_NULL_HANDLE,dispatch,stages,limits()),std::invalid_argument);
    REQUIRE_THROWS_AS(VulkanPipelineLayout(device,{},stages,limits()),std::invalid_argument);
    REQUIRE(invalid.attempts==0);
    tracker=nullptr;
}
