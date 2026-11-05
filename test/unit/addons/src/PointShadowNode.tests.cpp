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
TEST_CASE("PointShadowNode diagnoses the render primitive operations") {
    const auto vector = nativeVector3Constructor().construct();
    const auto matrix = nativeMatrix4Constructor().construct();
    const auto otherMatrix = nativeMatrix4Constructor().construct();
    const auto frustum = nativeFrustumConstructor().construct();

    REQUIRE_NOTHROW(vector.get("setFromMatrixPosition").call(vector, {matrix}));
    REQUIRE_NOTHROW(matrix.get("multiplyMatrices").call(matrix, {otherMatrix, otherMatrix}));
    // WebGLCoordinateSystem is 2000 in the pinned constants table; other values raise.
    REQUIRE_NOTHROW(frustum.get("setFromProjectionMatrix").call(
        frustum, {matrix, JSValue(2000), JSValue(false)}));
    REQUIRE_NOTHROW(matrix.get("makeTranslation").call(matrix, {JSValue(1), JSValue(2), JSValue(3)}));
}
TEST_CASE("PointShadowNode renders the six cube faces") {
    auto camera = JSValue::object(nullptr);
    camera.set("position", nativeVector3Constructor().construct());
    camera.set("up", nativeVector3Constructor().construct());
    camera.set("projectionMatrix", nativeMatrix4Constructor().construct());
    camera.set("matrixWorldInverse", nativeMatrix4Constructor().construct());
    camera.set("far", JSValue(100));
    // WebGLCoordinateSystem is 2000; the frustum helper validates the value.
    camera.set("coordinateSystem", JSValue(2000));
    camera.set("reversedDepth", false);
    for (const char* method : {"updateProjectionMatrix", "lookAt", "updateMatrixWorld"}) {
        camera.set(method, JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
                return JSValue{};
            }));
    }
    auto mapSize = JSValue::object(nullptr);
    mapSize.set("width", JSValue(64));
    auto shadow = JSValue::object(nullptr);
    shadow.set("camera", camera);
    shadow.set("matrix", nativeMatrix4Constructor().construct());
    shadow.set("_frustum", nativeFrustumConstructor().construct());
    shadow.set("mapSize", mapSize);
    shadow.set("clearColor", nativeColorConstructor().construct());
    shadow.set("clearAlpha", JSValue(1));
    auto light = JSValue::object(nullptr);
    light.set("shadow", shadow);
    light.set("matrixWorld", nativeMatrix4Constructor().construct());
    light.set("name", JSValue("Point"));
    light.set("distance", JSValue(0));
    light.set("id", JSValue(3));
    auto scene = JSValue::object(nullptr);
    scene.set("name", JSValue("Scene"));
    auto renderer = JSValue::object(nullptr);
    renderer.set("coordinateSystem", JSValue(2000));
    renderer.set("autoClear", true);
    renderer.set("getClearColor", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) {
            return args.size() > 0 ? args[0] : JSValue{};
        }));
    renderer.set("getClearAlpha", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue(1);
        }));
    for (const char* method : {"setClearColor", "clear"}) {
        renderer.set(method, JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
                return JSValue{};
            }));
    }
    // The target binding records the six face indices on the receiver.
    renderer.set("_faces", JSValue::array(0));
    renderer.set("_targetCalls", JSValue(0));
    renderer.set("setRenderTarget", JSValue::function(
        +[](const JSValue& receiver, const std::vector<JSValue>& args,
            const std::vector<JSValue>&) {
            JSValue& self = const_cast<JSValue&>(receiver);
            const auto count = static_cast<std::size_t>(self.get("_targetCalls").toNumber());
            self.set("_targetCalls", JSValue(static_cast<double>(count + 1)));
            auto faces = self.get("_faces");
            faces.set(count, args.size() > 1 ? args[1] : JSValue{});
            self.set("_faces", faces);
            return JSValue{};
        }));
    renderer.set("render", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    auto frame = JSValue::object(nullptr);
    frame.set("renderer", renderer);
    frame.set("scene", scene);

    auto node = nativePointShadowNodeConstructor().construct({light});
    auto target = JSValue::object(nullptr);
    target.set("setSize", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) {
            return JSValue{};
        }));
    node.set("shadowMap", target);
    REQUIRE_NOTHROW(node.get("renderShadow").call(node, {frame}));
    REQUIRE(scene.get("name").toString() == "Scene");
    REQUIRE(renderer.get("autoClear").toBoolean());
    // One target bind per cube face, in order.
    REQUIRE(renderer.get("_targetCalls").toNumber() == Approx(6));
    const auto recordedFaces = renderer.get("_faces");
    for (int face = 0; face < 6; ++face) {
        REQUIRE(recordedFaces.get(JSValue(static_cast<double>(face))).toNumber() ==
                Approx(static_cast<double>(face)));
    }
}