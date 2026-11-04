#include "nodes/accessors/StorageTextureNode.h"
#include "nodes/tsl/TSLCore.h"
#include "textures/Texture.h"

#include "catch.hpp"

TEST_CASE("StorageTextureNode exports the storage texture factories") {
    const auto module = nativeStorageTextureNodeModule();
    REQUIRE(module.get("default").sameValue(nativeStorageTextureNodeConstructor()));
    REQUIRE(module.get("storageTexture").isCallable());
    REQUIRE(module.get("textureStore").isCallable());
}

TEST_CASE("StorageTextureNode preserves access, coordinates and store metadata") {
    const auto texture = nativeTextureConstructor().construct();
    const auto uv = nativeTSLCoreBinding("vec2").call({}, {2, 3});
    const auto value = nativeTSLCoreBinding("vec4").call({}, {1, 0, 0, 1});
    const auto node = nativeStorageTextureNodeBinding("storageTexture").call({}, {texture, uv, value});
    REQUIRE(node.get("type").strictEquals("StorageTextureNode"));
    REQUIRE(node.get("isStorageTextureNode").strictEquals(true));
    REQUIRE(node.get("access").strictEquals("writeOnly"));
    REQUIRE(node.get("mipLevel").strictEquals(0));
    const auto unwrap = [](JSValue value) {
        for (int depth = 0; depth < 4 && value.get("type").strictEquals("VarNode"); ++depth)
            value = value.get("node");
        return value;
    };
    REQUIRE(unwrap(node.get("uvNode")).sameValue(unwrap(uv)));
    REQUIRE(unwrap(node.get("storeNode")).sameValue(unwrap(value)));
    REQUIRE(node.get("getInputType").call(node).strictEquals("storageTexture"));
    REQUIRE(node.get("getTransformedUV").call(node, {uv}).sameValue(uv));
    REQUIRE(node.get("setMipLevel").call(node, {2}).sameValue(node));
    REQUIRE(node.get("mipLevel").strictEquals(2));
    REQUIRE(node.get("toReadWrite").call(node).get("access").strictEquals("readWrite"));
    REQUIRE(node.get("toReadOnly").call(node).get("access").strictEquals("readOnly"));
    REQUIRE(node.get("toWriteOnly").call(node).get("access").strictEquals("writeOnly"));
    const auto clone = node.get("clone").call(node);
    REQUIRE(clone.get("type").strictEquals("StorageTextureNode"));
    REQUIRE(unwrap(clone.get("storeNode")).sameValue(unwrap(value)));
    REQUIRE(clone.get("mipLevel").strictEquals(2));
    REQUIRE(clone.get("access").strictEquals("writeOnly"));
}
