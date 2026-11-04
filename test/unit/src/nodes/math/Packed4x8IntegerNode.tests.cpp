#include "nodes/math/Packed4x8IntegerNode.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("Packed4x8IntegerNode exports all pinned packed integer helpers") {
    const auto module = nativePacked4x8IntegerNodeModule();
    REQUIRE(module.get("default").sameValue(nativePacked4x8IntegerNodeConstructor()));
    for (const auto* name : {"dot4U8Packed", "dot4I8Packed", "pack4xI8", "pack4xU8",
                             "pack4xI8Clamp", "pack4xU8Clamp", "unpack4xI8", "unpack4xU8"})
        REQUIRE(module.get(name).isCallable());
    REQUIRE(nativePacked4x8IntegerNodeConstructor().get("DOT4_U8_PACKED").strictEquals("dot4U8Packed"));
    REQUIRE(nativePacked4x8IntegerNodeConstructor().get("UNPACK4X_U8").strictEquals("unpack4xU8"));
}

TEST_CASE("Packed4x8IntegerNode preserves method, type and input metadata") {
    const auto uintNode = nativeTSLBaseBinding("uint").call({}, {7});
    const auto uvec = nativeTSLBaseBinding("uvec4").call({}, {1, 2, 3, 4});
    const auto ivec = nativeTSLBaseBinding("ivec4").call({}, {-1, 2, -3, 4});
    const auto dot = nativePacked4x8IntegerNodeBinding("dot4U8Packed").call({}, {uintNode, uintNode});
    const auto pack = nativePacked4x8IntegerNodeBinding("pack4xI8Clamp").call({}, {ivec});
    const auto unpack = nativePacked4x8IntegerNodeBinding("unpack4xU8").call({}, {uintNode});
    const auto dotNode = dot.get("isPacked4x8IntegerNode").strictEquals(true) ? dot : dot.get("node");
    const auto packNode = pack.get("isPacked4x8IntegerNode").strictEquals(true) ? pack : pack.get("node");
    const auto unpackNode = unpack.get("isPacked4x8IntegerNode").strictEquals(true) ? unpack : unpack.get("node");
    REQUIRE(dotNode.get("type").strictEquals("Packed4x8IntegerNode"));
    REQUIRE(dotNode.get("method").strictEquals("dot4U8Packed"));
    REQUIRE(dotNode.get("getInputType").call(dotNode).strictEquals("uint"));
    REQUIRE(dotNode.get("generateNodeType").call(dotNode, {JSValue::object()}).strictEquals("uint"));
    REQUIRE(packNode.get("method").strictEquals("pack4xI8Clamp"));
    REQUIRE(packNode.get("getInputType").call(packNode).strictEquals("ivec4"));
    REQUIRE(unpackNode.get("method").strictEquals("unpack4xU8"));
    REQUIRE(unpackNode.get("generateNodeType").call(unpackNode, {JSValue::object()}).strictEquals("uvec4"));
}
