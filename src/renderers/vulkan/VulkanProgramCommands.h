#pragma once
#include "core/JSValue.h"
#include "renderers/vulkan/VulkanProgramUniforms.h"
#include <cstdint>
#include <memory>

// Native shader/program commands for the original WebGLProgram/Uniforms
// protocols. The unchanged ESSL pair is parsed, linked and adapted by the
// native compiler. This owns reflection and uploaded uniform bytes; the
// renderer creates actual Vulkan modules/descriptors from that same result.
class VulkanProgramCommands {
  public:
    struct LinkedProgram {
        VulkanEsslResult result;
        std::unique_ptr<VulkanProgramUniforms> uniforms;
        std::uint64_t generation = 0, uniformRevision = 0;
        std::vector<VulkanEsslProgram::ActiveAttribute> activeAttributes;
    };
    VulkanProgramCommands();
    JSValue object() const;
    JSValue currentProgram() const;
    std::shared_ptr<LinkedProgram> linked(const JSValue& program) const;
    std::shared_ptr<LinkedProgram> current() const;
    void shutdown();
  private:
    JSValue state_, object_;
};
