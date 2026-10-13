#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

// Native glslang compilation of a linked Vulkan GLSL program. Source building,
// WebGLProgram state, reflection, GPU bindings and shader-module creation remain
// separate responsibilities. The compiler never executes JavaScript.
class VulkanShaderCompiler {
  public:
    struct Result {
        bool linked = false;
        int exitCode = -1;
        std::string diagnostics;
        std::vector<std::uint32_t> vertex, fragment;
    };
    explicit VulkanShaderCompiler(std::filesystem::path executable);
    Result compile(const std::string& vertex, const std::string& fragment) const;

  private:
    std::filesystem::path executable_;
};
