#pragma once
#include "renderers/vulkan/VulkanEsslProgram.h"
#include <span>
namespace threecppEssl {
VulkanEsslProgram compile(const VulkanEsslSourcePair& pair,
        std::span<const VulkanEsslAttributeBinding> attributes={});
VulkanEsslStageResult validateStage(VulkanEsslStage stage,const std::string& source,const std::string& name);
}
