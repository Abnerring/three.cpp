#include "nodes/lighting/BasicLightMapNode.h"

#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("BasicLightMapNode exposes the pinned lighting node shape") {
    const auto ctor = nativeBasicLightMapNodeConstructor();
    REQUIRE(ctor.get("name").strictEquals("BasicLightMapNode"));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("BasicLightMapNode"));
    REQUIRE(ctor.get("prototype").get("constructor").sameValue(ctor));
    REQUIRE(ctor.get("prototype").get("setup").get("length").strictEquals(1));
}

TEST_CASE("BasicLightMapNode setup stores reciprocal-pi irradiance light map") {
    const auto lightMap = nativeTSLBaseBinding("vec3").call({}, {1.0, 2.0, 3.0});
    const auto node = nativeBasicLightMapNodeConstructor().construct({lightMap});
    auto context = JSValue::object(nullptr);
    auto builder = JSValue::object(nullptr);
    builder.set("context", context);

    node.get("setup").call(node, {builder});
    const auto result = context.get("irradianceLightMap");
    REQUIRE(result.get("isNode").strictEquals(true));
    REQUIRE(result.get("type").strictEquals("VarNode"));
    const auto input = result.get("node");
    REQUIRE(input.get("type").strictEquals("OperatorNode"));
    REQUIRE(input.get("op").strictEquals("*"));
}
