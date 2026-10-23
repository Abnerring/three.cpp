#pragma once
#include <cstdint>
#include <string>

// Port of the rules in src/nodes/utils/CubeMapNode.js at the pinned revision. The node turns an
// equirectangular environment into a cube map, keeping one conversion per source texture.
class CubeMapNode {
  public:
    static constexpr const char* kTypeName = "CubeMapNode";
    static constexpr const char* kNodeType = "vec3";

    // Whether a mapping needs converting. Two of the equirectangular kinds do, the reflection
    // one and the refraction one, so a texture set up for either is converted rather than only
    // the more common of the two.
    static bool needsConversion(const std::string& mapping);

    // The kinds that need converting, named as the constants module names them.
    static constexpr const char* kEquirectReflection = "EquirectangularReflectionMapping";
    static constexpr const char* kEquirectRefraction = "EquirectangularRefractionMapping";

    // Where a texture comes from. The node accepts either a texture node, whose value is the
    // texture, or a reference to a property on the material, whose value is read from the
    // material by name.
    enum class Source { TextureNode, MaterialReference, None };

    static Source source(bool isTextureNode, bool isMaterialReferenceNode);

    // Whether a conversion is remembered per source texture. It is, so two nodes given the same
    // environment share one cube map rather than each building their own.
    static bool cachesPerTexture();

    // Whether the mapping is written onto a cached cube map again on each use. It is, so one
    // cached conversion can serve a reflection use and a refraction use without being rebuilt.
    static bool reappliesMappingOnHit();

    // The kind of refresh the node asks for, which is once per render rather than once per
    // frame: the conversion happens before a render that needs it.
    static constexpr const char* kUpdateBeforeType = "RENDER";

    // Whether the placeholder is a real texture rather than nothing. It is a cube texture marked
    // as belonging to a render target, so a shader that samples the node before a conversion has
    // finished gets a texture rather than a missing one.
    static constexpr bool placeholderIsRealTexture = true;

    // Whether the placeholder is marked as a render target texture, which a test checks because
    // the flag is what makes it usable as one.
    static constexpr bool placeholderIsRenderTargetTexture = true;

    // How many conversions are remembered at once. The record is keyed by texture, so the count
    // grows with the number of distinct environments rather than with the number of nodes.
    static std::uint32_t cachedCount();
    static std::uint64_t conversionFor(std::uint64_t textureId);
    static void reset();
};
