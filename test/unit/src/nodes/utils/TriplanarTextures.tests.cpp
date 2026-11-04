#include "nodes/utils/TriplanarTextures.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/Texture.h"

#include "catch.hpp"

TEST_CASE("TriplanarTextures exports both TSL aliases") {
    const auto module = nativeTriplanarTexturesModule();
    const auto textures = nativeTriplanarTextures();
    const auto textureAlias = nativeTriplanarTexture();
    REQUIRE(module.get("triplanarTextures").sameValue(textures));
    REQUIRE(module.get("triplanarTexture").sameValue(textureAlias));
    REQUIRE(textures.get("type").strictEquals("Node"));
    REQUIRE(textures.get("isFn").strictEquals(true));
    REQUIRE(textures.get("isNode").strictEquals(true));
    REQUIRE(textures.get("length").isCallable());
    REQUIRE(textureAlias.get("name").strictEquals("triplanarTexture"));
    REQUIRE(textureAlias.get("length").strictEquals(0));
}

TEST_CASE("TriplanarTextures builds a blended sample node") {
    const auto textureNode = nativeTextureNodeBinding("texture").call({}, {
        nativeTextureConstructor().construct()});
    const auto vec3 = nativeTSLBaseBinding("vec3");
    const auto floating = nativeTSLBaseBinding("float");
    const auto result = nativeTriplanarTextures().call({}, {
        textureNode, nullptr, nullptr, floating.call({}, {2}),
        vec3.call({}, {1, 2, 3}), vec3.call({}, {0, 1, 0})});
    REQUIRE(result.get("isNode").strictEquals(true));
    REQUIRE(result.get("node").get("rawInputs").get("length").strictEquals(6));
}
