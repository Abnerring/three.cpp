#include "nodes/accessors/StorageTexture3DNode.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/Texture.h"

#include "catch.hpp"

TEST_CASE("StorageTexture3DNode exports the three.js factory") {
    const auto module = nativeStorageTexture3DNodeModule();
    REQUIRE(module.get("default").sameValue(nativeStorageTexture3DNodeConstructor()));
    REQUIRE(module.get("storageTexture3D").isCallable());
}

TEST_CASE("StorageTexture3DNode preserves storage metadata and coordinate types") {
    const auto texture = nativeTextureConstructor().construct();
    const auto uv = nativeTSLBaseBinding("vec3").call({}, {1, 2, 3});
    const auto value = nativeTSLBaseBinding("vec4").call({}, {1, 0, 0, 1});
    const auto node = nativeStorageTexture3DNodeBinding("storageTexture3D").call({}, {texture, uv, value});

    REQUIRE(node.get("type").strictEquals("StorageTexture3DNode"));
    REQUIRE(node.get("isStorageTextureNode").strictEquals(true));
    REQUIRE(node.get("isStorageTexture3DNode").strictEquals(true));
    REQUIRE(node.get("getInputType").call(node).strictEquals("storageTexture"));
    REQUIRE(node.get("access").strictEquals("writeOnly"));
    REQUIRE(node.get("mipLevel").strictEquals(0));
    REQUIRE(node.get("updateMatrix").strictEquals(false));
    REQUIRE(node.get("uvNode").get("type").strictEquals("VarNode"));
    REQUIRE(node.get("storeNode").get("type").strictEquals("VarNode"));
    REQUIRE(node.get("getDefaultUV").call(node).get("type").strictEquals("VarNode"));
    REQUIRE(node.get("generateUV").isCallable());
    REQUIRE(node.get("generateOffset").isCallable());
}
