#include "nodes/core/CubeMapNode.h"

#include <map>

namespace {
// One conversion per source texture, keyed by the texture's identity.
std::map<std::uint64_t, std::uint64_t> gCache;
std::uint64_t gNextId = 1;
}  // namespace

bool CubeMapNode::needsConversion(const std::string& mapping) {
    // Both equirectangular kinds, the reflection one and the refraction one.
    return mapping == kEquirectReflection || mapping == kEquirectRefraction;
}

CubeMapNode::Source CubeMapNode::source(bool isTextureNode, bool isMaterialReferenceNode) {
    if (isTextureNode) return Source::TextureNode;
    if (isMaterialReferenceNode) return Source::MaterialReference;
    return Source::None;
}

bool CubeMapNode::cachesPerTexture() {
    return true;
}

bool CubeMapNode::reappliesMappingOnHit() {
    return true;
}

std::uint32_t CubeMapNode::cachedCount() {
    return static_cast<std::uint32_t>(gCache.size());
}

std::uint64_t CubeMapNode::conversionFor(std::uint64_t textureId) {
    const auto found = gCache.find(textureId);
    if (found != gCache.end()) return found->second;
    const std::uint64_t made = gNextId;
    gNextId += 1;
    gCache[textureId] = made;
    return made;
}

void CubeMapNode::reset() {
    gCache.clear();
    gNextId = 1;
}
