#include "nodes/utils/SpriteSheetUV.h"

#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("spritesheetUV exports the pinned Fn") {
    const auto module = nativeSpriteSheetUVModule();
    const auto fn = nativeSpriteSheetUV();
    REQUIRE(module.get("spritesheetUV").sameValue(fn));
    REQUIRE(fn.get("type").strictEquals("Node"));
    REQUIRE(fn.get("isFn").toBoolean());
    REQUIRE(fn.get("shaderNode").get("type").strictEquals("Node"));
}

TEST_CASE("spritesheetUV preserves the three optional inputs") {
    const auto count = nativeTSLBaseBinding("vec2").call({}, {6, 6});
    const auto uv = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
    const auto frame = nativeTSLBaseBinding("float").call({}, {7});
    const auto result = nativeSpriteSheetUV().call({}, {count, uv, frame});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
    const auto rawInputs = result.get("node").get("rawInputs");
    REQUIRE(rawInputs.get("length").toNumber() == 3);
    REQUIRE(rawInputs.get("0").get("type").strictEquals("VarNode"));
    REQUIRE(rawInputs.get("1").get("type").strictEquals("VarNode"));
    REQUIRE(rawInputs.get("2").get("type").strictEquals("VarNode"));
}

TEST_CASE("spritesheetUV accepts omitted UV and frame inputs") {
    const auto count = nativeTSLBaseBinding("vec2").call({}, {6, 6});
    const auto result = nativeSpriteSheetUV().call({}, {count});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("rawInputs").get("length").toNumber() == 1);
}
