#pragma once
#include "renderers/vulkan/VulkanEsslProgram.h"
#include "renderers/vulkan/VulkanUniformBuffer.h"
#include <compare>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <vector>

// Owned uniform storage for one linked ESSL program generation. The caller
// evaluates/converts application values before invoking this native layer.
// Complete descriptor blocks determine allocation; active uniform mappings
// determine application names. Private draw values have a separate entry point.
class VulkanProgramUniforms {
  public:
    struct Key {
        std::uint32_t set = 0, binding = 0;
        auto operator<=>(const Key&) const = default;
    };
    struct Block {
        Key key;
        std::uint32_t stages;
        VulkanShaderInterface::Block layout;
        VulkanUniformBuffer storage;
    };
    struct Sampler {
        Key key;
        std::uint32_t stages;
        std::vector<std::int32_t> units;
    };
    explicit VulkanProgramUniforms(const VulkanEsslResult& program);
    const std::map<Key, Block>& blocks() const noexcept { return blocks_; }
    const std::map<std::string, Sampler>& samplers() const noexcept { return samplers_; }
    std::span<const std::byte> bytes(Key key) const;
    void write(const std::string& name, std::span<const float> values);
    void write(const std::string& name, std::span<const std::int32_t> values);
    void write(const std::string& name, std::span<const std::uint32_t> values);
    void writeBooleans(const std::string& name, std::span<const std::int32_t> values);
    void writeSamplerUnits(const std::string& name, std::span<const std::int32_t> values);
    void writeBackend(std::uint32_t firstInstance, float framebufferHeight);

  private:
    struct UniformRoot { Key key; };
    struct Backend {
        VulkanEsslResult::BackendScalar::Value value;
        Key key;
        std::string name;
    };
    std::map<Key, Block> blocks_;
    std::map<std::string, UniformRoot> roots_;
    std::map<std::string, Sampler> samplers_;
    std::vector<Backend> backend_;
    VulkanUniformBuffer& buffer(const std::string& name);
};
