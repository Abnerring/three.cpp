#include "nodes/accessors/TextureBicubic.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/Texture.h"

#include "catch.hpp"

TEST_CASE("TextureBicubic exports both filtering functions") {
    const auto module = nativeTextureBicubicModule();
    REQUIRE(module.get("textureBicubicLevel").get("isFn").toBoolean());
    REQUIRE(module.get("textureBicubic").get("isFn").toBoolean());
    REQUIRE(module.get("textureBicubicLevel").sameValue(nativeTextureBicubicLevel()));
    REQUIRE(module.get("textureBicubic").sameValue(nativeTextureBicubic()));
}

TEST_CASE("TextureBicubic builds the filtered sample graph") {
    const auto texture = nativeTextureConstructor().construct();
    const auto uv = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.5});
    const auto textureNode = nativeTextureNodeBinding("texture").call({}, {texture, uv});
    const auto lod = nativeTSLBaseBinding("float").call({}, {0.5});

    const auto level = nativeTextureBicubicLevel().call({}, {textureNode, lod});
    const auto filtered = nativeTextureBicubic().call({}, {textureNode, lod});
    REQUIRE(level.get("type").strictEquals("VarNode"));
    REQUIRE(filtered.get("type").strictEquals("VarNode"));
    REQUIRE(level.get("node").get("type").toBoolean());
    REQUIRE(filtered.get("node").get("type").toBoolean());
}
