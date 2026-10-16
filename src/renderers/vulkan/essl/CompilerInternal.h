#pragma once
#include "renderers/vulkan/VulkanEsslProgram.h"
#include <span>
#include <glslang/Public/ShaderLang.h>
#include <glslang/MachineIndependent/localintermediate.h>
namespace threecppEssl {
VulkanEsslProgram compile(const VulkanEsslSourcePair& pair,
        std::span<const VulkanEsslAttributeBinding> attributes);
VulkanEsslStageResult validateStage(VulkanEsslStage stage,const std::string& source,const std::string& name);
void mapAst(glslang::TProgram& program, VulkanEsslProgram& result, std::span<const VulkanEsslAttributeBinding> attributes);
VulkanEsslProgram::Module emit(const glslang::TIntermediate& intermediate);
}
