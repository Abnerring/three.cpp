#include "renderers/vulkan/VulkanShaderModules.h"
#include <limits>
#include <stdexcept>
#include <utility>

VulkanShaderModules::VulkanShaderModules(VkDevice device, Dispatch dispatch, std::span<const Stage> stages)
    : device_(device), dispatch_(dispatch) {
    if (!device_ || !dispatch_.create || !dispatch_.destroy || stages.empty())
        throw std::invalid_argument("Shader modules require a live device, dispatch and compiled stages");
    // Finish reflection and every potentially allocating container operation
    // before the first native allocation. Invalid entry points cannot leak GPU
    // resources, and the caller's input buffers are needed only during creation.
    modules_.reserve(stages.size());
    interfaces_.reserve(stages.size());
    entryPoints_.reserve(stages.size());
    for (const auto& stage : stages) {
        if (stage.words.size() > std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t))
            throw std::length_error("Shader module byte size overflow");
        interfaces_.push_back(VulkanShaderInterface::read(stage.words, stage.entryPoint));
        entryPoints_.push_back(stage.entryPoint);
    }
    try {
        for (const auto& stage : stages) {
            VkShaderModuleCreateInfo create{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            create.codeSize = stage.words.size() * sizeof(std::uint32_t);
            create.pCode = stage.words.data();
            VkShaderModule module = VK_NULL_HANDLE;
            const auto result = dispatch_.create(device_, &create, nullptr, &module);
            if (result != VK_SUCCESS)
                throw std::runtime_error("vkCreateShaderModule failed: " + std::to_string(result));
            modules_.push_back(module);
        }
    } catch (...) {
        reset();
        throw;
    }
}
VulkanShaderModules::~VulkanShaderModules() { reset(); }
VulkanShaderModules::VulkanShaderModules(VulkanShaderModules&& other) noexcept {
    *this = std::move(other);
}
VulkanShaderModules& VulkanShaderModules::operator=(VulkanShaderModules&& other) noexcept {
    if (this != &other) {
        reset();
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        dispatch_ = std::exchange(other.dispatch_, {});
        modules_ = std::move(other.modules_);
        interfaces_ = std::move(other.interfaces_);
        entryPoints_ = std::move(other.entryPoints_);
    }
    return *this;
}
void VulkanShaderModules::reset() noexcept {
    if (device_)
        for (auto entry = modules_.rbegin(); entry != modules_.rend(); ++entry)
            dispatch_.destroy(device_, *entry, nullptr);
    modules_.clear();
    interfaces_.clear();
    entryPoints_.clear();
    device_ = VK_NULL_HANDLE;
    dispatch_ = {};
}
std::vector<VkPipelineShaderStageCreateInfo> VulkanShaderModules::pipelineStages() const {
    std::vector<VkPipelineShaderStageCreateInfo> result;
    result.reserve(modules_.size());
    for (std::size_t index = 0; index < modules_.size(); ++index) {
        VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stage.stage = static_cast<VkShaderStageFlagBits>(interfaces_[index].stage);
        stage.module = modules_[index];
        stage.pName = entryPoints_[index].c_str();
        result.push_back(stage);
    }
    return result;
}
