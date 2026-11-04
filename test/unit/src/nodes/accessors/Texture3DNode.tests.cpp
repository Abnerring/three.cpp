#include "nodes/accessors/Texture3DNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/Texture.h"

#include "catch.hpp"

TEST_CASE("Texture3DNode exports the texture3D factories") {
    const auto module = nativeTexture3DNodeModule();
    REQUIRE(module.get("default").sameValue(nativeTexture3DNodeConstructor()));
    REQUIRE(module.get("texture3D").isCallable());
    REQUIRE(module.get("texture3DLoad").isCallable());
    REQUIRE(module.get("texture3DLevel").isCallable());
}

TEST_CASE("Texture3DNode preserves three.js metadata and coordinate types") {
    const auto texture = nativeTextureConstructor().construct();
    const auto uv = nativeTSLBaseBinding("vec3").call({}, {.25, .5, .75});
    const auto level = nativeTSLBaseBinding("float").call({}, {2});
    const auto texture3D = nativeTexture3DNodeBinding("texture3D");
    const auto node = texture3D.call({}, {texture, uv});
    REQUIRE(node.get("type").strictEquals("Texture3DNode"));
    REQUIRE(node.get("isTexture3DNode").strictEquals(true));
    REQUIRE(node.get("getInputType").call(node).strictEquals("texture3D"));
    auto defaultUv = node.get("getDefaultUV").call(node);
    REQUIRE(defaultUv.get("type").strictEquals("VarNode"));
    REQUIRE(node.get("getSampler").call(node).strictEquals(true));
    REQUIRE(node.get("updateMatrix").strictEquals(false));
    REQUIRE(node.get("uvNode").get("type").strictEquals("VarNode"));

    const auto loaded = nativeTexture3DNodeBinding("texture3DLoad").call({}, {texture, uv});
    REQUIRE(loaded.get("getSampler").call(loaded).strictEquals(false));
    const auto levelNode = nativeTexture3DNodeBinding("texture3DLevel").call({}, {texture, uv, level});
    REQUIRE(levelNode.get("levelNode").get("type").strictEquals("VarNode"));
    const auto unwrap = [](JSValue value) {
        for (int depth = 0; depth < 4 && value.get("type").strictEquals("VarNode"); ++depth)
            value = value.get("node");
        return value;
    };
    REQUIRE(unwrap(levelNode.get("levelNode")).sameValue(unwrap(level)));
}

TEST_CASE("Texture3DNode emits ivec3 coordinates for load and offsets") {
    const auto texture = nativeTextureConstructor().construct();
    const auto uv = nativeTSLCoreBinding("vec3").call({}, {.25, .5, .75});
    const auto node = nativeTexture3DNodeBinding("texture3DLoad").call({}, {texture, uv});
    REQUIRE(node.get("generateUV").isCallable());
    REQUIRE(node.get("generateOffset").isCallable());
    REQUIRE(node.get("normal").isCallable());
}
