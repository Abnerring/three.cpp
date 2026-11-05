#include "nodes/utils/UVUtils.h"

#include "nodes/tsl/TSLBase.h"
#include "catch.hpp"

TEST_CASE("UVUtils exports all pinned helpers") {
    const auto module = nativeUVUtilsModule();
    REQUIRE(module.get("replaceDefaultUV").isCallable());
    REQUIRE(module.get("rotateUV").get("isFn").strictEquals(true));
    REQUIRE(module.get("spherizeUV").get("isFn").strictEquals(true));
    REQUIRE(nativeReplaceDefaultUV().sameValue(module.get("replaceDefaultUV")));
    REQUIRE(nativeRotateUV().sameValue(module.get("rotateUV")));
    REQUIRE(nativeSpherizeUV().sameValue(module.get("spherizeUV")));
}

TEST_CASE("UVUtils builds rotation and spherize nodes with optional centers") {
    const auto uv = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
    const auto rotation = nativeTSLBaseBinding("float").call({}, {0.5});
    const auto rotated = nativeRotateUV().call({}, {uv, rotation});
    const auto warped = nativeSpherizeUV().call({}, {uv, 1.0});
    REQUIRE(rotated.get("isNode").strictEquals(true));
    REQUIRE(warped.get("isNode").strictEquals(true));
    REQUIRE(rotated.get("node").get("rawInputs").get("length").strictEquals(2));
    REQUIRE(warped.get("node").get("rawInputs").get("length").strictEquals(2));
}

TEST_CASE("UVUtils replaceDefaultUV preserves callback and context node") {
    const auto callback = JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return {}; }, {}, "callback", 1);
    const auto node = nativeTSLBaseBinding("vec2").call({}, {0.1, 0.2});
    const auto context = nativeReplaceDefaultUV().call({}, {callback, node});
    REQUIRE(context.get("isContextNode").strictEquals(true));
    REQUIRE(context.get("node").sameValue(node));
    REQUIRE(context.get("value").get("getUV").isCallable());
}
