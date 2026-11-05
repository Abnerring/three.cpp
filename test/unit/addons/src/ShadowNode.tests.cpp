#include "nodes/lighting/ShadowNode.h"

#include "core/JSValue.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

#include <stdexcept>

namespace {
JSValue makeLight() {
    auto shadow = JSValue::object(nullptr);
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);
    return light;
}
}  // namespace

TEST_CASE("ShadowNode constructor applies the pinned fields") {
    auto light = makeLight();
    const auto node = nativeShadowNodeConstructor().construct({light});
    REQUIRE(node.get("type").toString() == "ShadowNode");
    REQUIRE(node.get("shadow").strictEquals(light.get("shadow")));
    REQUIRE(node.get("shadowMap").isNull());
    REQUIRE(node.get("vsmShadowMapVertical").isNull());
    REQUIRE(node.get("vsmShadowMapHorizontal").isNull());
    REQUIRE(node.get("vsmMaterialVertical").isNull());
    REQUIRE(node.get("vsmMaterialHorizontal").isNull());
    REQUIRE(node.get("_node").isNull());
    REQUIRE(node.get("_currentShadowType").isNull());
    REQUIRE(node.get("isShadowNode").toBoolean());
    REQUIRE(node.get("depthLayer").toNumber() == Approx(0));
}

TEST_CASE("ShadowNode exposes the pinned shadow filter library") {
    const auto node = nativeShadowNodeConstructor().construct({makeLight()});
    const auto filterFn = node.get("getShadowFilterFn");
    REQUIRE(filterFn.toBoolean());
    REQUIRE(filterFn.call(node, {JSValue(0.0)}).toBoolean());
    REQUIRE(filterFn.call(node, {JSValue(1.0)}).toBoolean());
    REQUIRE(filterFn.call(node, {JSValue(2.0)}).isNull());
    REQUIRE(filterFn.call(node, {JSValue(3.0)}).toBoolean());
}

TEST_CASE("ShadowNode dispose runs the reset path on a fresh node") {
    const auto node = nativeShadowNodeConstructor().construct({makeLight()});
    REQUIRE_NOTHROW(node.get("dispose").call(node, {}));
    REQUIRE(node.get("shadowMap").isNull());
    REQUIRE(node.get("_currentShadowType").isNull());
}

TEST_CASE("ShadowNode heavy methods raise until their round lands") {
    const auto node = nativeShadowNodeConstructor().construct({makeLight()});
    REQUIRE_THROWS_AS(node.get("setupShadow").call(node, {JSValue::object(nullptr)}),
                      std::runtime_error);
}
TEST_CASE("ShadowNode setupShadowCoord returns a node for the orthographic path") {
    // A real TSL vec4 as the projected shadow position, and a supplied bias node so the reference
    // helper is not needed.
    const auto shadowPosition = nativeTSLBaseBinding("vec4").call(
        {}, {JSValue(0.25), JSValue(0.5), JSValue(0.75), JSValue(1.0)});
    const auto bias = nativeTSLBaseBinding("float").call({}, {JSValue(0.0)});
    auto camera = JSValue::object(nullptr);
    camera.set("isOrthographicCamera", true);
    auto shadow = JSValue::object(nullptr);
    shadow.set("camera", camera);
    shadow.set("biasNode", bias);
    auto renderer = JSValue::object(nullptr);
    renderer.set("logarithmicDepthBuffer", false);
    auto builder = JSValue::object(nullptr);
    builder.set("renderer", renderer);
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);

    const auto node = nativeShadowNodeConstructor().construct({light});
    const auto result = node.get("setupShadowCoord").call(node, {builder, shadowPosition});
    REQUIRE(result.toBoolean());
    REQUIRE(result.get("isNode").toBoolean());
}