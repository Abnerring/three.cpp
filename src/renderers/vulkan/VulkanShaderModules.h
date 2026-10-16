#pragma once
#include "renderers/vulkan/VulkanShaderInterface.h"
#include <span>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

// Owns compiled shader modules on a borrowed live device. Destroy the owner
// before the device. Reflection and entry-point strings are independent of the
// input SPIR-V buffers; no graphics/compute pipeline is owned here.
class VulkanShaderModules {
  public:
    struct Stage {
        std::span<const std::uint32_t> words;
        std::string entryPoint = "main";
    };
    struct Dispatch {
        PFN_vkCreateShaderModule create = nullptr;
        PFN_vkDestroyShaderModule destroy = nullptr;
    };
    VulkanShaderModules(VkDevice device, Dispatch dispatch, std::span<const Stage> stages);
    ~VulkanShaderModules();
    VulkanShaderModules(const VulkanShaderModules&) = delete;
    VulkanShaderModules& operator=(const VulkanShaderModules&) = delete;
    VulkanShaderModules(VulkanShaderModules&&) noexcept;
    VulkanShaderModules& operator=(VulkanShaderModules&&) noexcept;
    void reset() noexcept;
    std::span<const VkShaderModule> handles() const noexcept { return modules_; }
    std::span<const VulkanShaderInterface> interfaces() const noexcept { return interfaces_; }
    // Returned pName pointers borrow this owner's immutable entry-point strings.
    // Rebuild these descriptions after moving or resetting the owner.
    std::vector<VkPipelineShaderStageCreateInfo> pipelineStages() const;

  private:
    VkDevice device_ = VK_NULL_HANDLE;
    Dispatch dispatch_;
    std::vector<VkShaderModule> modules_;
    std::vector<VulkanShaderInterface> interfaces_;
    std::vector<std::string> entryPoints_;
};
