#include "nodes/math/UnpackFloatNode.h"
#include "nodes/tsl/TSLCore.h"

#include "catch.hpp"

TEST_CASE("UnpackFloatNode exports all pinned unpacking helpers") {
    const auto module = nativeUnpackFloatNodeModule();
    for (const auto& name : {"unpackSnorm2x16", "unpackUnorm2x16", "unpackHalf2x16", "unpackSnorm4x8", "unpackUnorm4x8"}) {
        REQUIRE(module.get(name).isCallable());
    }
    REQUIRE(module.get("default").sameValue(nativeUnpackFloatNodeConstructor()));
}

TEST_CASE("UnpackFloatNode preserves encoding, layout and output type") {
    const auto uintNode = nativeTSLCoreBinding("uint").call({}, {42});
    const auto snorm = nativeUnpackFloatNodeBinding("unpackSnorm2x16").call({}, {uintNode});
    const auto unorm = nativeUnpackFloatNodeBinding("unpackUnorm4x8").call({}, {uintNode});
    const auto snormNode = snorm.get("isUnpackFloatNode").strictEquals(true) ? snorm : snorm.get("node");
    const auto unormNode = unorm.get("isUnpackFloatNode").strictEquals(true) ? unorm : unorm.get("node");
    REQUIRE(snormNode.get("type").strictEquals("UnpackFloatNode"));
    REQUIRE(snormNode.get("encoding").strictEquals("snorm"));
    REQUIRE(snormNode.get("layout").strictEquals("2x16"));
    REQUIRE(snormNode.get("isUnpackFloatNode").strictEquals(true));
    REQUIRE(unormNode.get("encoding").strictEquals("unorm"));
    REQUIRE(unormNode.get("layout").strictEquals("4x8"));
    REQUIRE(snormNode.get("generateNodeType").call(snormNode, {JSValue::object()}).strictEquals("vec2"));
    REQUIRE(unormNode.get("generateNodeType").call(unormNode, {JSValue::object()}).strictEquals("vec4"));
}
