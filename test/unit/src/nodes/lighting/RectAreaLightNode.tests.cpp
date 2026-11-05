#include "nodes/lighting/RectAreaLightNode.h"

#include "math/Color.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/UniformNode.h"
#include "textures/Texture.h"

#include "catch.hpp"

namespace {
// A light the rect area node can read. Its matrixWorld is a ninety degree turn about the y axis, so a
// width vector along x has to come out along negative z after the rotation is extracted.
JSValue makeLight(const JSValue& matrixWorld) {
    auto light = JSValue::object(nullptr);
    auto color = nativeColorConstructor().construct();
    color.set("r", 1.0); color.set("g", 1.0); color.set("b", 1.0);
    light.set("color", color);
    light.set("intensity", 1);
    light.set("matrixWorld", matrixWorld);
    light.set("width", 4);
    light.set("height", 2);
    return light;
}
JSValue turnAboutY() {
    const auto matrix = nativeMatrix4Constructor().construct();
    // Row major: x maps to negative z and z maps to x.
    matrix.get("set").call(matrix, {JSValue(0), JSValue(0), JSValue(1), JSValue(0),
                                    JSValue(0), JSValue(1), JSValue(0), JSValue(0),
                                    JSValue(-1), JSValue(0), JSValue(0), JSValue(0),
                                    JSValue(0), JSValue(0), JSValue(0), JSValue(1)});
    return matrix;
}
JSValue identityMatrix() {
    const auto matrix = nativeMatrix4Constructor().construct();
    matrix.get("identity").call(matrix, {});
    return matrix;
}
JSValue frame() {
    auto camera = JSValue::object(nullptr);
    camera.set("matrixWorldInverse", identityMatrix());
    auto value = JSValue::object(nullptr);
    value.set("camera", camera);
    return value;
}
}  // namespace

TEST_CASE("RectAreaLightNode exposes the pinned rect area shape") {
    const auto light = makeLight(turnAboutY());
    const auto module = nativeRectAreaLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("RectAreaLightNode"));
    // constructor( light = null ) has a default, so Function.length is 0.
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("RectAreaLightNode"));
    REQUIRE(ctor.get("setLTC").isCallable());

    const auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("RectAreaLightNode"));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    for (const char* name : {"halfWidth", "halfHeight"}) {
        REQUIRE(node.get(name).get("type").strictEquals("UniformNode"));
        REQUIRE(node.get(name).get("groupNode").get("name").strictEquals("render"));
        REQUIRE(node.get(name).get("value").get("isVector3").strictEquals(true));
    }
    // The update type is render, not frame, because the view matrix can change per render call.
    REQUIRE(node.get("updateType").strictEquals("render"));
    REQUIRE(node.get("setupDirectRectArea").get("length").strictEquals(1));
}

TEST_CASE("RectAreaLightNode update turns the half extents by the extracted view rotation") {
    const auto light = makeLight(turnAboutY());
    const auto node = nativeRectAreaLightNodeConstructor().construct({light});
    node.get("update").call(node, {frame()});

    // Half of the width goes along x and then, rotated by ninety degrees about y, along negative z.
    const auto halfWidth = node.get("halfWidth").get("value");
    REQUIRE(halfWidth.get("x").toNumber() == Approx(0.0).margin(1e-12));
    REQUIRE(halfWidth.get("y").toNumber() == Approx(0.0).margin(1e-12));
    REQUIRE(halfWidth.get("z").toNumber() == Approx(-2.0));

    // Half of the height stays along y because the rotation is about y.
    const auto halfHeight = node.get("halfHeight").get("value");
    REQUIRE(halfHeight.get("x").toNumber() == Approx(0.0).margin(1e-12));
    REQUIRE(halfHeight.get("y").toNumber() == Approx(1.0));
    REQUIRE(halfHeight.get("z").toNumber() == Approx(0.0).margin(1e-12));
}

TEST_CASE("RectAreaLightNode setupDirectRectArea answers the filter inputs") {
    auto library = JSValue::object(nullptr);
    for (const char* key : {"LTC_FLOAT_1", "LTC_FLOAT_2", "LTC_HALF_1", "LTC_HALF_2"}) {
        library.set(key, nativeTextureConstructor().construct());
    }
    nativeRectAreaLightNodeBinding("default").get("setLTC").call({}, {library});

    const auto light = makeLight(turnAboutY());
    const auto node = nativeRectAreaLightNodeConstructor().construct({light});

    const auto filtered = JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) -> JSValue {
            return captures[0];
        },
        {JSValue(true)}, "isAvailable", 1);
    auto builder = JSValue::object(nullptr);
    builder.set("isAvailable", filtered);
    const auto result = node.get("setupDirectRectArea").call(node, {builder});

    // The six fields the source builds.
    for (const char* key : {"lightColor", "lightPosition", "halfWidth", "halfHeight", "ltc_1", "ltc_2"}) {
        REQUIRE_FALSE(result.get(key).isUndefined());
    }
    REQUIRE(result.get("ltc_1").get("isNode").strictEquals(true));
    REQUIRE(result.get("ltc_2").get("isNode").strictEquals(true));
    REQUIRE_FALSE(result.get("ltc_1").sameValue(result.get("ltc_2")));
    // The half extents are handed through as the node's own uniforms.
    REQUIRE(result.get("halfWidth").sameValue(node.get("halfWidth")));
    REQUIRE(result.get("halfHeight").sameValue(node.get("halfHeight")));
    REQUIRE(result.get("lightColor").sameValue(node.get("colorNode")));

    // The filterable flag selects the float pair, and the node wraps exactly that texture.
    REQUIRE(result.get("ltc_1").get("value").sameValue(library.get("LTC_FLOAT_1")));
    REQUIRE(result.get("ltc_2").get("value").sameValue(library.get("LTC_FLOAT_2")));

    // Without float32 filtering the half precision pair is used instead.
    auto notFiltered = JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) -> JSValue {
            return captures[0];
        },
        {JSValue(false)}, "isAvailable", 1);
    auto halfBuilder = JSValue::object(nullptr);
    halfBuilder.set("isAvailable", notFiltered);
    const auto halfResult = node.get("setupDirectRectArea").call(node, {halfBuilder});
    REQUIRE(halfResult.get("ltc_1").get("value").sameValue(library.get("LTC_HALF_1")));
    REQUIRE(halfResult.get("ltc_2").get("value").sameValue(library.get("LTC_HALF_2")));
}
