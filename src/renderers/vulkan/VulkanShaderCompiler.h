#pragma once
#include "renderers/vulkan/VulkanEsslProgram.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <span>
#include <vector>

// Native linked compilation. The existing compile method uses the 450 CLI path;
// optional compileESSL parses unchanged ES source, adapts its AST and returns
// owned SPIR-V/reflection. Source building, program state, GPU resource binding
// and shader-module creation remain separate. No JavaScript is executed.
class VulkanShaderCompiler {
  public:
    struct Result {
        bool linked = false;
        int exitCode = -1;
        std::string diagnostics;
        std::vector<std::uint32_t> vertex, fragment;
    };
    // Available when built with THREECPP_NATIVE_ESSL. Does not require the CLI constructor.
#ifdef THREECPP_HAS_NATIVE_ESSL
    using ESSLResult = VulkanEsslResult;
    static ESSLResult compileESSL(const VulkanEsslSourcePair& pair);
    static ESSLResult compileESSL(const VulkanEsslSourcePair& pair,
                                  std::span<const VulkanEsslAttributeBinding> attributes);
    static VulkanEsslStageResult validateESSLStage(VulkanEsslStage stage,
            const std::string& source, const std::string& name="shader.essl");
#endif
    explicit VulkanShaderCompiler(std::filesystem::path executable);
    Result compile(const std::string& vertex, const std::string& fragment) const;

  private:
    std::filesystem::path executable_;
};
