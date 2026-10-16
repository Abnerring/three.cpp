#pragma once
#include "renderers/vulkan/VulkanShaderInterface.h"
#include <vulkan/vulkan.h>
#include <map>
#include <span>
#include <utility>
#include <vector>

// Internal Vulkan resources. The borrowed device and its dispatch must outlive
// this object; reset before destroying the device. No descriptor pools or GPU
// submissions are owned here. Shader buffers retain their per-stage layouts.
class VulkanPipelineLayout {
  public:
    using DescriptorCounts = std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t>;
    struct Description {
        std::vector<std::vector<VkDescriptorSetLayoutBinding>> sets;
        std::vector<VkPushConstantRange> pushConstants;
    };
    struct Dispatch {
        PFN_vkCreateDescriptorSetLayout createSet = nullptr;
        PFN_vkDestroyDescriptorSetLayout destroySet = nullptr;
        PFN_vkCreatePipelineLayout createPipeline = nullptr;
        PFN_vkDestroyPipelineLayout destroyPipeline = nullptr;
    };
    static Description describe(std::span<const VulkanShaderInterface> stages,
                                const VkPhysicalDeviceLimits& limits,
                                const DescriptorCounts& runtimeCounts = {},
                                std::uint32_t framebufferColorAttachments = 0);
    VulkanPipelineLayout(VkDevice device, Dispatch dispatch,
                         std::span<const VulkanShaderInterface> stages,
                         const VkPhysicalDeviceLimits& limits,
                         const DescriptorCounts& runtimeCounts = {},
                         std::uint32_t framebufferColorAttachments = 0);
    ~VulkanPipelineLayout();
    VulkanPipelineLayout(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout& operator=(const VulkanPipelineLayout&) = delete;
    VulkanPipelineLayout(VulkanPipelineLayout&&) noexcept;
    VulkanPipelineLayout& operator=(VulkanPipelineLayout&&) noexcept;
    void reset() noexcept;
    VkPipelineLayout handle() const noexcept { return pipeline_; }
    std::span<const VkDescriptorSetLayout> setLayouts() const noexcept { return sets_; }
    const Description& description() const noexcept { return description_; }
  private:
    VkDevice device_ = VK_NULL_HANDLE;
    Dispatch dispatch_;
    Description description_;
    std::vector<VkDescriptorSetLayout> sets_;
    VkPipelineLayout pipeline_ = VK_NULL_HANDLE;
};
