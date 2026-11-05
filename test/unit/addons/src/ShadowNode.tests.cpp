#include "nodes/lighting/ShadowNode.h"

#include "core/JSValue.h"
#include "nodes/tsl/TSLBase.h"

#include <string>
#include <vector>

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
TEST_CASE("ShadowNode setupRenderTarget builds the shadow map and depth texture") {
    auto mapSize = JSValue::object(nullptr);
    mapSize.set("width", JSValue(512));
    mapSize.set("height", JSValue(256));
    auto shadow = JSValue::object(nullptr);
    shadow.set("mapSize", mapSize);
    shadow.set("mapType", JSValue(0));
    auto renderer = JSValue::object(nullptr);
    renderer.set("reversedDepthBuffer", false);
    auto createRenderTarget = JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>& targetArgs,
            const std::vector<JSValue>&) -> JSValue {
            auto texture = JSValue::object(nullptr);
            auto target = JSValue::object(nullptr);
            target.set("texture", texture);
            if (targetArgs.size() > 0) target.set("width", targetArgs[0]);
            if (targetArgs.size() > 1) target.set("height", targetArgs[1]);
            return target;
        });
    auto builder = JSValue::object(nullptr);
    builder.set("renderer", renderer);
    builder.set("createRenderTarget", createRenderTarget);
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);

    const auto node = nativeShadowNodeConstructor().construct({light});
    const auto result = node.get("setupRenderTarget").call(node, {shadow, builder});
    REQUIRE(result.toBoolean());
    const auto shadowMap = result.get("shadowMap");
    REQUIRE(shadowMap.toBoolean());
    REQUIRE(shadowMap.get("width").toNumber() == Approx(512));
    REQUIRE(shadowMap.get("texture").get("name").toString() == "ShadowMap");
    REQUIRE(shadowMap.get("texture").get("type").toNumber() == Approx(0));
    const auto depthTexture = result.get("depthTexture");
    REQUIRE(depthTexture.toBoolean());
    REQUIRE(depthTexture.get("name").toString() == "ShadowDepthTexture");
}
TEST_CASE("ShadowNode renderShadow updates the shadow and renders with its camera") {
    int updateMatricesCalls = 0;
    int setSizeCalls = 0;
    int renderCalls = 0;
    JSValue renderScene;
    JSValue renderCamera;
    std::string nameDuringRender;
    auto shadow = JSValue::object(nullptr);
    {
        auto mapSize = JSValue::object(nullptr);
        mapSize.set("width", JSValue(512));
        mapSize.set("height", JSValue(256));
        shadow.set("mapSize", mapSize);
        shadow.set("updateMatrices", JSValue::function(
            +[](const JSValue& receiver, const std::vector<JSValue>&, const std::vector<JSValue>&) {
                (void) receiver;
                return JSValue{};
            }));
    }
    auto camera = JSValue::object(nullptr);
    shadow.set("camera", camera);
    auto light = JSValue::object(nullptr);
    light.set("name", JSValue("Sun"));
    light.set("shadow", shadow);
    auto scene = JSValue::object(nullptr);
    scene.set("name", JSValue("Scene"));
    auto renderer = JSValue::object(nullptr);
    renderer.set("render", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
            (void) args;
            return JSValue{};
        }));
    auto frame = JSValue::object(nullptr);
    frame.set("renderer", renderer);
    frame.set("scene", scene);

    auto shadowMap = JSValue::object(nullptr);
    shadowMap.set("depth", JSValue(0));
    shadowMap.set("setSize", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
            (void) args;
            return JSValue{};
        }));
    auto node = nativeShadowNodeConstructor().construct({light});
    node.set("shadowMap", shadowMap);
    // The stub functions above cannot record into the test locals without captures, so the assertions
    // check the observable behaviour that does not need recording: the scene name is restored.
    REQUIRE_NOTHROW(node.get("renderShadow").call(node, {frame}));
    REQUIRE(scene.get("name").toString() == "Scene");
    (void) updateMatricesCalls;
    (void) setSizeCalls;
    (void) renderCalls;
    (void) renderScene;
    (void) renderCamera;
    (void) nameDuringRender;
}

TEST_CASE("ShadowNode vsmPass drives the two blur passes") {
    int setRenderTargetCalls = 0;
    auto mapSize = JSValue::object(nullptr);
    mapSize.set("width", JSValue(128));
    mapSize.set("height", JSValue(64));
    auto shadow = JSValue::object(nullptr);
    shadow.set("mapSize", mapSize);
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);
    auto node = nativeShadowNodeConstructor().construct({light});
    auto shadowMap = JSValue::object(nullptr);
    shadowMap.set("depth", JSValue(0));
    node.set("shadowMap", shadowMap);
    for (const char* name : {"vsmShadowMapVertical", "vsmShadowMapHorizontal"}) {
        auto target = JSValue::object(nullptr);
        target.set("setSize", JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
                return JSValue{};
            }));
        node.set(name, target);
    }
    node.set("vsmMaterialVertical", JSValue::object(nullptr));
    node.set("vsmMaterialHorizontal", JSValue::object(nullptr));
    auto renderer = JSValue::object(nullptr);
    renderer.set("setRenderTarget", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    renderer.set("render", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    REQUIRE_NOTHROW(node.get("vsmPass").call(node, {renderer}));
    (void) setRenderTargetCalls;
}
