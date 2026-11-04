#include "nodes/shapes/Shapes.h"

#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("Shapes exports the pinned shapeCircle Fn") {
    const auto module = nativeShapesModule();
    const auto value = nativeShapeCircle();
    REQUIRE(module.get("shapeCircle").sameValue(value));
    REQUIRE(value.get("type").strictEquals("Node"));
    REQUIRE(value.get("isFn").strictEquals(true));
    REQUIRE(value.get("isNode").strictEquals(true));
    REQUIRE(value.get("length").isCallable());
    REQUIRE(value.get("shaderNode").get("jsFunc").toString().find("alphaToCoverage") != std::string::npos);
}

TEST_CASE("shapeCircle accepts the default UV and explicit coordinates") {
    const auto shape = nativeShapeCircle();
    const auto coord = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
    const auto value = shape.call({}, {coord});
    REQUIRE(value.get("isNode").strictEquals(true));
    REQUIRE(value.get("node").get("rawInputs").get("length").strictEquals(1));
    const auto defaultValue = shape.call();
    REQUIRE(defaultValue.get("isNode").strictEquals(true));
}
