#include "nodes/core/CubeMapNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("CubeMapNode converts both equirectangular kinds") {
    // The reflection kind and the refraction kind both need converting, so a texture set up for
    // either is handled; a test that only checked the more common one would miss half the cases.
    REQUIRE(CubeMapNode::needsConversion(CubeMapNode::kEquirectReflection));
    REQUIRE(CubeMapNode::needsConversion(CubeMapNode::kEquirectRefraction));
    REQUIRE(std::string(CubeMapNode::kEquirectReflection) == "EquirectangularReflectionMapping");
    REQUIRE(std::string(CubeMapNode::kEquirectRefraction) == "EquirectangularRefractionMapping");
    REQUIRE(std::string(CubeMapNode::kEquirectReflection) !=
            std::string(CubeMapNode::kEquirectRefraction));
}

TEST_CASE("CubeMapNode converts nothing else") {
    // A mapping that is not one of the two equirectangular kinds is left alone.
    REQUIRE_FALSE(CubeMapNode::needsConversion("CubeReflectionMapping"));
    REQUIRE_FALSE(CubeMapNode::needsConversion("UVMapping"));
    REQUIRE_FALSE(CubeMapNode::needsConversion(""));
    REQUIRE(std::string(CubeMapNode::kNodeType) == "vec3");
    REQUIRE(std::string(CubeMapNode::kTypeName) == "CubeMapNode");
}

TEST_CASE("CubeMapNode takes a texture from one of two places") {
    // A texture node's value is the texture itself; a material reference is read by name.
    REQUIRE(CubeMapNode::source(true, false) == CubeMapNode::Source::TextureNode);
    REQUIRE(CubeMapNode::source(false, true) == CubeMapNode::Source::MaterialReference);
    // Neither gives nothing.
    REQUIRE(CubeMapNode::source(false, false) == CubeMapNode::Source::None);
    // The texture node wins when both are true, since it is checked first.
    REQUIRE(CubeMapNode::source(true, true) == CubeMapNode::Source::TextureNode);
}

TEST_CASE("CubeMapNode keeps one conversion per source texture") {
    CubeMapNode::reset();
    // A first conversion makes one.
    const auto first = CubeMapNode::conversionFor(7);
    REQUIRE(CubeMapNode::cachedCount() == 1);
    // Asking again for the same texture gives the same conversion rather than a new one.
    REQUIRE(CubeMapNode::conversionFor(7) == first);
    REQUIRE(CubeMapNode::cachedCount() == 1);
    // A different texture gets its own.
    const auto second = CubeMapNode::conversionFor(9);
    REQUIRE(second != first);
    REQUIRE(CubeMapNode::cachedCount() == 2);
    REQUIRE(CubeMapNode::cachesPerTexture());
    CubeMapNode::reset();
}

TEST_CASE("CubeMapNode writes the mapping again on a cached conversion") {
    // A cached conversion can serve a reflection use and a refraction use, so the mapping is
    // written onto it each time rather than only when it was built.
    REQUIRE(CubeMapNode::reappliesMappingOnHit());
    // The refresh is once per render rather than once per frame.
    REQUIRE(std::string(CubeMapNode::kUpdateBeforeType) == "RENDER");
    REQUIRE(std::string(CubeMapNode::kUpdateBeforeType) != "FRAME");
    // The placeholder is a real texture marked as belonging to a render target, so a shader
    // sampling it before a conversion has finished gets something rather than nothing.
    REQUIRE(CubeMapNode::placeholderIsRealTexture);
    REQUIRE(CubeMapNode::placeholderIsRenderTargetTexture);
}