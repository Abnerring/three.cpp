#include "nodes/lighting/PointShadowNode.h"

#include "core/JSValue.h"

#include "catch.hpp"

#include "core/NativeObject.h"
#include "nodes/tsl/TSLBase.h"
#include "textures/CubeDepthTexture.h"

#include <memory>

#include "math/Color.h"
#include "math/Frustum.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"

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
TEST_CASE("PointShadowNode setupShadowFilter builds the point filter graph") {
    const auto shadowCoord = nativeTSLBaseBinding("vec4").call(
        {}, {JSValue(0.5), JSValue(0.5), JSValue(0.5), JSValue(1.0)});
    auto shadow = JSValue::object(nullptr);
    shadow.set("camera", JSValue::object(nullptr));
    shadow.set("radius", JSValue(1.0));
    shadow.set("mapSize", JSValue::object(nullptr));
    auto depthTexture = nativeTexture(std::make_shared<CubeDepthTexture>(64.0));
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);
    const auto node = nativePointShadowNodeConstructor().construct({light});
    const auto filterFn = node.get("getShadowFilterFn").call(node, {JSValue(0)});
    auto configuration = JSValue::object(nullptr);
    configuration.set("filterFn", filterFn);
    configuration.set("depthTexture", depthTexture);
    configuration.set("shadowCoord", shadowCoord);
    configuration.set("shadow", shadow);
    auto renderer = JSValue::object(nullptr);
    renderer.set("reversedDepthBuffer", false);
    auto builder = JSValue::object(nullptr);
    builder.set("renderer", renderer);
    const auto result = node.get("setupShadowFilter").call(node, {builder, configuration});
    REQUIRE(result.toBoolean());
    REQUIRE(result.get("isNode").toBoolean());
}
TEST_CASE("PointShadowNode point filter selects the depth conversion branch") {
    const auto shadowCoord = nativeTSLBaseBinding("vec4").call(
        {}, {JSValue(0.5), JSValue(0.5), JSValue(0.5), JSValue(1.0)});
    auto shadow = JSValue::object(nullptr);
    auto shadowCamera = JSValue::object(nullptr);
    shadowCamera.set("near", JSValue(0.1));
    shadowCamera.set("far", JSValue(100.0));
    shadow.set("camera", shadowCamera);
    shadow.set("radius", JSValue(1.0));
    auto mapSize = JSValue::object(nullptr);
    mapSize.set("x", JSValue(64));
    mapSize.set("y", JSValue(64));
    shadow.set("mapSize", mapSize);
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);
    const auto node = nativePointShadowNodeConstructor().construct({light});
    const auto filterFn = node.get("getShadowFilterFn").call(node, {JSValue(0)});
    const auto depthTexture = nativeTexture(std::make_shared<CubeDepthTexture>(64.0));

    for (const int branch : {0, 1, 2}) {
        auto renderer = JSValue::object(nullptr);
        renderer.set("reversedDepthBuffer", branch == 1);
        renderer.set("logarithmicDepthBuffer", branch == 2);
        auto builder = JSValue::object(nullptr);
        builder.set("renderer", renderer);
        auto configuration = JSValue::object(nullptr);
        configuration.set("filterFn", filterFn);
        configuration.set("depthTexture", depthTexture);
        configuration.set("shadowCoord", shadowCoord);
        configuration.set("shadow", shadow);
        const auto result = node.get("setupShadowFilter").call(node, {builder, configuration});
        REQUIRE(result.toBoolean());
        REQUIRE(result.get("isNode").toBoolean());
    }
}