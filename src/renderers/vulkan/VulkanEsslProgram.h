#pragma once
#include "renderers/vulkan/VulkanShaderInterface.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Native owned data. No glslang types, native pointers, JS execution, GPU handles,
// or filesystem outputs cross the compiler boundary. Stage bits: vertex=1, fragment=2.
enum class VulkanEsslStage { Vertex, Fragment };
struct VulkanEsslStageResult {
    bool compiled=false;
    int originalVersion=0, originalProfile=0;
    std::string phase="initialize", diagnostics;
};
struct VulkanEsslAttributeBinding { std::string name; std::uint32_t location=0; };
struct VulkanEsslSourcePair {
    std::string vertex, fragment;
    std::string vertexName = "vertex.essl", fragmentName = "fragment.essl";
};
struct VulkanEsslProgram {
    struct ActiveUniform {
        std::string name;
        int glType=0, size=0, block=-1, offset=-1, arrayStride=0;
        std::uint32_t stages=0;
    };
    struct ActiveAttribute { std::string name; int glType=0, size=0; };
    struct Uniform { std::string name, type; };
    struct Sampler { std::string name; std::uint32_t set=0, binding=0; };
    struct Module {
        std::vector<std::uint32_t> words;
        bool validated=false;
        std::string validator, logger, assembly;
    };
    bool linked=false;
    std::string phase="initialize", diagnostics;
    std::array<int,2> originalVersions{}, originalProfiles{};
    std::vector<ActiveUniform> activeUniforms;
    std::vector<ActiveAttribute> activeAttributes;
    std::vector<Uniform> uniforms;
    std::vector<Sampler> samplers;
    std::array<Module,2> modules;
};
struct VulkanEsslResult : VulkanEsslProgram {
    struct UniformMapping {
        enum class Kind { BlockMember, Sampler };
        std::string sourceName, rootName, memberPath;
        Kind kind=Kind::BlockMember;
        // stages describes emitted declarations, not browser active-location parity.
        std::uint32_t set=0, binding=0, stages=0;
        // Reflected member tree for the source uniform's rootName. Scalar/matrix
        // roots are leaves, not complete UBOs. Allocate storage from the matching
        // interfaces[stage].descriptors(set,binding).block, then address values
        // with rootName+memberPath and actual offsets/strides. Never construct a
        // block buffer from this member alone. memberPath retains source spelling.
        VulkanShaderInterface::Block rootLayout;
    };
    struct AttributeMapping {
        std::string sourceName;
        int glType=0, size=0;
        std::uint32_t location=~0u, component=0;
        // Original glslang AST reflection can include builtin inputs such as
        // gl_VertexID; these map to a SPIR-V builtin, not a vertex-buffer slot.
        int builtIn=-1;
        VulkanShaderInterface::Numeric numeric;
        VulkanShaderInterface::Array array;
    };
    struct VaryingMapping {
        std::string name;
        std::uint32_t vertexLocation=~0u, fragmentLocation=~0u;
        bool vertexDeclared=false, fragmentDeclared=false;
    };
    struct BackendScalar {
        enum class Value { FirstInstance, FramebufferHeight };
        Value value=Value::FirstInstance;
        std::string name;
        std::uint32_t set=0, binding=0, offset=~0u, size=0;
        // A block can contain this member without any instruction reading it.
        std::uint32_t declaredStages=0, usedStages=0;
        VulkanShaderInterface::Numeric numeric;
    };
    std::array<VulkanShaderInterface,2> interfaces;
    std::vector<UniformMapping> uniformMappings;
    std::vector<AttributeMapping> attributeMappings;
    std::vector<VaryingMapping> varyingMappings;
    std::vector<BackendScalar> backendScalars;
    // Pair compilation uses the portable wrapper. Renderer contract: positive
    // Vulkan viewport height, top=framebufferHeight-glY-glHeight, matching front
    // face convention, actual firstInstance and actual framebuffer height.
};
