#include "nodes/math/PackFloatNode.h"
#include "nodes/tsl/TSLCore.h"

#include "catch.hpp"

TEST_CASE("PackFloatNode exports all pinned packing helpers") {
    const auto module = nativePackFloatNodeModule();
    for (const auto& name : {"packSnorm2x16", "packUnorm2x16", "packHalf2x16", "packSnorm4x8", "packUnorm4x8"}) {
        REQUIRE(module.get(name).isCallable());
    }
    REQUIRE(module.get("default").sameValue(nativePackFloatNodeConstructor()));
}

TEST_CASE("PackFloatNode preserves encoding, layout and uint output") {
    const auto vec2 = nativeTSLCoreBinding("vec2").call({}, {0.1, 0.2});
    const auto vec4 = nativeTSLCoreBinding("vec4").call({}, {0.1, 0.2, 0.3, 0.4});
    const auto snorm = nativePackFloatNodeBinding("packSnorm2x16").call({}, {vec2});
    const auto unorm = nativePackFloatNodeBinding("packUnorm4x8").call({}, {vec4});
    const auto snormNode = snorm.get("isPackFloatNode").strictEquals(true) ? snorm : snorm.get("node");
    const auto unormNode = unorm.get("isPackFloatNode").strictEquals(true) ? unorm : unorm.get("node");
    REQUIRE(snormNode.get("type").strictEquals("PackFloatNode"));
    REQUIRE(snormNode.get("encoding").strictEquals("snorm"));
    REQUIRE(snormNode.get("layout").strictEquals("2x16"));
    REQUIRE(snormNode.get("isPackFloatNode").strictEquals(true));
    REQUIRE(unormNode.get("encoding").strictEquals("unorm"));
    REQUIRE(unormNode.get("layout").strictEquals("4x8"));
    REQUIRE(snormNode.get("generateNodeType").call(snormNode, {JSValue::object()}).strictEquals("uint"));
}
