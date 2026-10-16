#include "renderers/vulkan/VulkanProgramUniforms.h"
#include <algorithm>
#include <bit>
#include <charconv>
#include <stdexcept>
#include <tuple>

namespace {
using Interface = VulkanShaderInterface;
using Block = Interface::Block;
using Key = VulkanProgramUniforms::Key;
std::string rootName(const std::string& name) {
    return name.substr(0, name.find_first_of(".["));
}
bool sameLayout(const Block& a, const Block& b) {
    const auto& x = a.numeric;
    const auto& y = b.numeric;
    if (std::tie(a.name, a.offset, a.absoluteOffset, a.size, a.paddedSize) !=
        std::tie(b.name, b.offset, b.absoluteOffset, b.size, b.paddedSize) ||
        std::tie(x.scalar, x.width, x.components, x.columns, x.rows, x.matrixStride, x.rowMajor) !=
        std::tie(y.scalar, y.width, y.components, y.columns, y.rows, y.matrixStride, y.rowMajor) ||
        a.array.dimensions != b.array.dimensions || a.array.strides != b.array.strides ||
        a.array.stride != b.array.stride || a.array.specializationIds != b.array.specializationIds ||
        a.members.size() != b.members.size()) return false;
    for (std::size_t i = 0; i < a.members.size(); ++i)
        if (!sameLayout(a.members[i], b.members[i])) return false;
    return true;
}
const Interface::Descriptor& descriptor(const VulkanEsslResult& program, Key key, unsigned stage) {
    const auto& ds = program.interfaces[stage].descriptors;
    const auto found = std::find_if(ds.begin(), ds.end(), [&](const auto& d) {
        return d.set == key.set && d.binding == key.binding;
    });
    if (found == ds.end()) throw std::invalid_argument("Uniform mapping has no descriptor");
    return *found;
}
const Block& member(const Block& block, const std::string& name) {
    const auto found = std::find_if(block.members.begin(), block.members.end(), [&](const auto& b) {
        return b.name == name;
    });
    if (found == block.members.end()) throw std::invalid_argument("Uniform mapping has no block member");
    return *found;
}
}

VulkanProgramUniforms::VulkanProgramUniforms(const VulkanEsslResult& program) {
    if (!program.linked) throw std::invalid_argument("Uniform storage requires a linked ESSL program");
    for (unsigned stage = 0; stage < 2; ++stage) {
        for (const auto& d : program.interfaces[stage].descriptors) {
            if (d.type != 6) continue; // VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            if (d.count != 1) throw std::invalid_argument("Uniform block descriptor arrays are not supported");
            const Key key{d.set, d.binding};
            if (const auto old = blocks_.find(key); old != blocks_.end()) {
                if (!sameLayout(old->second.layout, d.block))
                    throw std::invalid_argument("Cross-stage complete uniform block layout mismatch");
                old->second.stages |= 1u << stage;
            } else {
                blocks_.emplace(key, Block{key, 1u << stage, d.block, VulkanUniformBuffer(d.block)});
            }
        }
    }
    for (const auto& mapping : program.uniformMappings) {
        if (mapping.rootName.empty() || rootName(mapping.sourceName) != mapping.rootName ||
            mapping.sourceName != mapping.rootName + mapping.memberPath ||
            !mapping.stages || (mapping.stages & ~3u))
            throw std::invalid_argument("Invalid original uniform name or stage mapping");
        const Key key{mapping.set, mapping.binding};
        if (mapping.kind == VulkanEsslResult::UniformMapping::Kind::Sampler) {
            std::uint32_t count = 0;
            for (unsigned stage = 0; stage < 2; ++stage) if (mapping.stages & (1u << stage)) {
                const auto& d = descriptor(program, key, stage);
                if (d.type != 1 || !d.count) // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                    throw std::invalid_argument("Unsupported original sampler descriptor");
                if (count && count != d.count) throw std::invalid_argument("Cross-stage sampler array mismatch");
                count = d.count;
            }
            const auto old = samplers_.find(mapping.rootName);
            if (old != samplers_.end()) {
                if (old->second.key != key || old->second.units.size() != count)
                    throw std::invalid_argument("Conflicting sampler mapping");
                old->second.stages |= mapping.stages;
            } else {
                samplers_.emplace(mapping.rootName, Sampler{key, mapping.stages, std::vector<std::int32_t>(count)});
            }
        } else {
            const auto block = blocks_.find(key);
            if (block == blocks_.end() || (mapping.stages & ~block->second.stages))
                throw std::invalid_argument("Original numeric uniform has no full block storage");
            if (!sameLayout(member(block->second.layout, mapping.rootName), mapping.rootLayout))
                throw std::invalid_argument("Original uniform member layout mismatch");
            const auto old = roots_.find(mapping.rootName);
            if (old != roots_.end() && old->second.key != key)
                throw std::invalid_argument("Conflicting original uniform root binding");
            roots_.insert_or_assign(mapping.rootName, UniformRoot{key});
        }
    }
    for (const auto& mapped : program.backendScalars) {
        if (!mapped.declaredStages) continue;
        if ((mapped.declaredStages & ~3u) || (mapped.usedStages & ~mapped.declaredStages))
            throw std::invalid_argument("Invalid backend scalar stages");
        const Key key{mapped.set, mapped.binding};
        const auto block = blocks_.find(key);
        if (block == blocks_.end() || (mapped.declaredStages & ~block->second.stages))
            throw std::invalid_argument("Backend scalar has no full block storage");
        const auto& layout = member(block->second.layout, mapped.name);
        const auto scalar = mapped.value == VulkanEsslResult::BackendScalar::Value::FirstInstance
            ? Interface::Numeric::Scalar::SignedInteger : Interface::Numeric::Scalar::FloatingPoint;
        if (layout.numeric.scalar != scalar || layout.numeric.width != 32 || layout.size != 4 ||
            layout.absoluteOffset != mapped.offset || mapped.size != 4 || !layout.array.dimensions.empty() ||
            roots_.contains(mapped.name) || samplers_.contains(mapped.name))
            throw std::invalid_argument("Invalid or exposed private backend scalar");
        backend_.push_back({mapped.value, key, mapped.name});
    }
}

std::span<const std::byte> VulkanProgramUniforms::bytes(Key key) const {
    return blocks_.at(key).storage.bytes();
}
VulkanUniformBuffer& VulkanProgramUniforms::buffer(const std::string& name) {
    const auto root = roots_.find(rootName(name));
    if (root == roots_.end()) throw std::invalid_argument("Unknown original numeric uniform: " + name);
    return blocks_.at(root->second.key).storage;
}
void VulkanProgramUniforms::write(const std::string& name, std::span<const float> values) {
    buffer(name).write(name, values);
}
void VulkanProgramUniforms::write(const std::string& name, std::span<const std::int32_t> values) {
    buffer(name).write(name, values);
}
void VulkanProgramUniforms::write(const std::string& name, std::span<const std::uint32_t> values) {
    buffer(name).write(name, values);
}
void VulkanProgramUniforms::writeBooleans(const std::string& name, std::span<const std::int32_t> values) {
    buffer(name).writeBooleans(name, values);
}
void VulkanProgramUniforms::writeSamplerUnits(const std::string& name, std::span<const std::int32_t> values) {
    const auto root = rootName(name);
    const auto found = samplers_.find(root);
    if (found == samplers_.end()) throw std::invalid_argument("Unknown original sampler uniform: " + name);
    std::size_t first = 0;
    if (name != root) {
        if (name.size() < root.size() + 3 || name[root.size()] != '[' || name.back() != ']')
            throw std::invalid_argument("Invalid original sampler element path");
        const auto begin = name.data() + root.size() + 1;
        const auto end = name.data() + name.size() - 1;
        const auto parsed = std::from_chars(begin, end, first);
        if (parsed.ec != std::errc{} || parsed.ptr != end)
            throw std::invalid_argument("Invalid original sampler element index");
    }
    auto& units = found->second.units;
    if (first >= units.size() || values.size() > units.size() - first ||
        std::any_of(values.begin(), values.end(), [](auto unit) { return unit < 0; }))
        throw std::invalid_argument("Sampler unit count or value mismatch");
    // Values can alias this generation's current assignments.
    const std::vector<std::int32_t> copy(values.begin(), values.end());
    std::copy(copy.begin(), copy.end(), units.begin() + first);
}
void VulkanProgramUniforms::writeBackend(std::uint32_t firstInstance, float framebufferHeight) {
    const std::int32_t base = std::bit_cast<std::int32_t>(firstInstance);
    for (const auto& scalar : backend_) {
        auto& storage = blocks_.at(scalar.key).storage;
        if (scalar.value == VulkanEsslResult::BackendScalar::Value::FirstInstance)
            storage.write(scalar.name, std::span(&base, 1));
        else storage.write(scalar.name, std::span(&framebufferHeight, 1));
    }
}
