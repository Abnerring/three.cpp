#pragma once
#include "renderers/vulkan/VulkanShaderInterface.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

// CPU storage for a reflected block. Inputs are already converted GPU scalars,
// tightly packed and column-major; this layer never reads application objects.
// Offsets/strides come from SPIR-V, including arrays and row-major matrices.
class VulkanUniformBuffer {
  public:
    using Scalar = VulkanShaderInterface::Numeric::Scalar;
    using Dimensions = std::map<std::string, std::vector<std::uint32_t>>;
    VulkanUniformBuffer(const VulkanShaderInterface::Block& block,
                        std::size_t byteSize = 0, const Dimensions& dimensions = {});
    std::span<const std::byte> bytes() const noexcept { return storage_; }
    void writeRaw(const std::string& name, Scalar scalar, std::uint32_t width,
                  std::span<const std::byte> packed);
    void write(const std::string& name, std::span<const float> values);
    void write(const std::string& name, std::span<const double> values);
    void write(const std::string& name, std::span<const std::int32_t> values);
    void write(const std::string& name, std::span<const std::uint32_t> values);
    // GLSL booleans in buffers can be lowered to uint by the compiler.
    void writeBooleans(const std::string& name, std::span<const std::int32_t> values);
  private:
    struct Plan {
        Scalar scalar;
        std::uint32_t width;
        std::size_t components;
        std::shared_ptr<const std::vector<std::size_t>> offsets;
        std::size_t first = 0;
    };
    std::vector<std::byte> storage_;
    std::map<std::string, Plan> plans_;
    void expand(const VulkanShaderInterface::Block& node, const std::string& name,
                std::size_t base, const Dimensions& dimensions);
};
