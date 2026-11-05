#include "nodes/lighting/PointShadowNode.h"

#include "core/JSValue.h"

#include "catch.hpp"

namespace {
JSValue makeLight() {
    auto shadow = JSValue::object(nullptr);
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);
    return light;
}
}  // namespace

TEST_CASE("PointShadowNode extends the shadow node") {
    const auto node = nativePointShadowNodeConstructor().construct({makeLight()});
    REQUIRE(node.get("isPointShadowNode").toBoolean());
    REQUIRE(node.get("isShadowNode").toBoolean());
    REQUIRE(node.get("type").toString() == "PointShadowNode");
}

TEST_CASE("PointShadowNode setupShadowCoord passes the position through") {
    const auto node = nativePointShadowNodeConstructor().construct({makeLight()});
    auto position = JSValue::object(nullptr);
    position.set("isNode", true);
    const auto result = node.get("setupShadowCoord").call(node, {JSValue::object(nullptr), position});
    REQUIRE(result.strictEquals(position));
}

TEST_CASE("PointShadowNode setupRenderTarget builds the cube target") {
    auto mapSize = JSValue::object(nullptr);
    mapSize.set("width", JSValue(256));
    auto shadow = JSValue::object(nullptr);
    shadow.set("mapSize", mapSize);
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);
    auto renderer = JSValue::object(nullptr);
    renderer.set("reversedDepthBuffer", false);
    auto builder = JSValue::object(nullptr);
    builder.set("renderer", renderer);
    builder.set("createCubeRenderTarget", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            auto target = JSValue::object(nullptr);
            target.set("texture", JSValue::object(nullptr));
            return target;
        }));
    const auto node = nativePointShadowNodeConstructor().construct({light});
    const auto result = node.get("setupRenderTarget").call(node, {shadow, builder});
    REQUIRE(result.toBoolean());
    REQUIRE(result.get("shadowMap").get("texture").get("name").toString() == "PointShadowMap");
    REQUIRE(result.get("depthTexture").get("name").toString() == "PointShadowDepthTexture");
}

TEST_CASE("PointShadowNode basic filter samples the cube depth texture") {
    const auto node = nativePointShadowNodeConstructor().construct({makeLight()});
    // BasicShadowMap is 0 in the pinned constants table.
    const auto filter = node.get("getShadowFilterFn").call(node, {JSValue(0)});
    REQUIRE(filter.isCallable());
    auto depthTexture = JSValue::object(nullptr);
    auto bd3D = JSValue::object(nullptr);
    auto dp = JSValue::object(nullptr);
    auto inputs = JSValue::object(nullptr);
    inputs.set("depthTexture", depthTexture);
    inputs.set("bd3D", bd3D);
    inputs.set("dp", dp);
    // The stub textures still answer a node for the cube compare chain.
    REQUIRE(filter.call({}, {inputs}).toBoolean());
}