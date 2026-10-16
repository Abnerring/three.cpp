#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

// Owned reflection of compiled SPIR-V. Vulkan enum values are stored without
// exposing a Vulkan header dependency. This is not a SPIR-V validator.
struct VulkanShaderInterface {
    struct Numeric {
        enum class Scalar { Unknown, Boolean, SignedInteger, UnsignedInteger, FloatingPoint };
        Scalar scalar = Scalar::Unknown;
        std::uint32_t width = 0, components = 0, columns = 0, rows = 0, matrixStride = 0;
        bool rowMajor = false;
    };
    struct Array {
        std::vector<std::uint32_t> dimensions, specializationIds;
        std::uint32_t stride = 0;
        // One decorated stride per dimension, outermost first.
        std::vector<std::uint32_t> strides;
    };
    struct Block {
        std::string name, typeName;
        std::uint32_t offset = 0, absoluteOffset = 0, size = 0, paddedSize = 0;
        bool used = false;
        Numeric numeric;
        Array array;
        std::vector<Block> members;
    };
    struct Descriptor {
        std::string name, typeName;
        std::uint32_t set = 0, binding = 0, type = 0, count = 0;
        std::vector<std::uint32_t> arrayDimensions;
        std::uint32_t imageDimension = 0, imageFormat = 0, imageDepth = 0, imageSampled = 0;
        bool imageArray = false, imageMultisampled = false;
        Block block;
    };
    struct Variable {
        std::string name, typeName;
        std::uint32_t location = ~0u, component = 0, format = 0;
        int builtIn = -1;
        Numeric numeric;
        Array array;
        std::vector<Variable> members;
    };
    std::string entryPoint;
    std::uint32_t stage = 0;
    std::array<std::uint32_t, 3> localSize{};
    std::vector<Descriptor> descriptors;
    std::vector<Variable> inputs, outputs;
    std::vector<Block> pushConstants;
    static VulkanShaderInterface read(std::span<const std::uint32_t> words,
                                      const std::string& entryPoint = "main");
};
