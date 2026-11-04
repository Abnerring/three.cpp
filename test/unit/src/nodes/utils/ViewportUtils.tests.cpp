#include "nodes/utils/ViewportUtils.h"

#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("ViewportUtils exports the pinned viewportSafeUV function") {
    const auto module = nativeViewportUtilsModule();
    const auto value = nativeViewportSafeUV();
    REQUIRE(module.get("viewportSafeUV").sameValue(value));
    REQUIRE(value.get("type").strictEquals("Node"));
    REQUIRE(value.get("isFn").strictEquals(true));
    REQUIRE(value.get("isNode").strictEquals(true));
    REQUIRE(value.get("length").isCallable());
    REQUIRE(value.get("shaderNode").get("jsFunc").toString().find("viewportDepthTexture") != std::string::npos);
}

TEST_CASE("ViewportUtils accepts optional coordinates and builds the safe UV selection") {
    const auto value = nativeViewportSafeUV();
    const auto coord = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
    const auto explicitCall = value.call({}, {coord});
    const auto defaultCall = value.call();
    REQUIRE(explicitCall.get("isNode").strictEquals(true));
    REQUIRE(defaultCall.get("isNode").strictEquals(true));
    REQUIRE(explicitCall.get("node").get("rawInputs").get("length").strictEquals(1));
    REQUIRE(defaultCall.get("node").get("rawInputs").get("length").strictEquals(0));
}
