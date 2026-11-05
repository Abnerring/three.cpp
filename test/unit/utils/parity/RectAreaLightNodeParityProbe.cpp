#include "nodes/lighting/RectAreaLightNode.h"

#include "core/JSValue.h"
#include "math/Color.h"
#include "math/Matrix4.h"
#include "nodes/core/Node.h"
#include "textures/Texture.h"

#include <iostream>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue turnAboutY() {
    const auto matrix = nativeMatrix4Constructor().construct();
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
}  // namespace

int main() {
    try {
        const auto module = nativeRectAreaLightNodeModule();
        const auto ctor = nativeRectAreaLightNodeConstructor();

        auto light = JSValue::object(nullptr);
        auto color = nativeColorConstructor().construct();
        color.set("r", 1.0); color.set("g", 1.0); color.set("b", 1.0);
        light.set("color", color);
        light.set("intensity", 1);
        light.set("matrixWorld", turnAboutY());
        light.set("width", 4);
        light.set("height", 2);

        const auto node = ctor.construct({light});
        auto camera = JSValue::object(nullptr);
        camera.set("matrixWorldInverse", identityMatrix());
        auto frame = JSValue::object(nullptr);
        frame.set("camera", camera);
        node.get("update").call(node, {frame});
        const auto halfWidth = node.get("halfWidth").get("value");
        const auto halfHeight = node.get("halfHeight").get("value");

        // The filter inputs are configured through the static setter, as the renderer does.
        auto library = JSValue::object(nullptr);
        for (const char* key : {"LTC_FLOAT_1", "LTC_FLOAT_2", "LTC_HALF_1", "LTC_HALF_2"}) {
            library.set(key, nativeTextureConstructor().construct());
        }
        ctor.get("setLTC").call({}, {library});
        auto builder = JSValue::object(nullptr);
        builder.set("isAvailable", JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
                return captures[0];
            },
            {JSValue(true)}, "isAvailable", 1));
        const auto result = node.get("setupDirectRectArea").call(node, {builder});
        auto halfBuilder = JSValue::object(nullptr);
        halfBuilder.set("isAvailable", JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
                return captures[0];
            },
            {JSValue(false)}, "isAvailable", 1));
        const auto halfResult = node.get("setupDirectRectArea").call(node, {halfBuilder});

        auto keys = JSValue::array();
        for (const char* key : {"lightColor", "lightPosition", "halfWidth", "halfHeight", "ltc_1", "ltc_2"}) {
            keys.set(keys.length(), JSValue(key));
        }

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("instanceType", node.get("type"));
        output.set("isAnalyticLightNode", node.get("isAnalyticLightNode"));
        output.set("updateType", node.get("updateType"));
        output.set("halfWidthX", halfWidth.get("x"));
        output.set("halfWidthY", halfWidth.get("y"));
        output.set("halfWidthZ", halfWidth.get("z"));
        output.set("halfHeightX", halfHeight.get("x"));
        output.set("halfHeightY", halfHeight.get("y"));
        output.set("halfHeightZ", halfHeight.get("z"));
        output.set("keys", keys);
        output.set("ltc1IsNode", result.get("ltc_1").get("isNode"));
        output.set("ltc2IsNode", result.get("ltc_2").get("isNode"));
        output.set("ltcDistinct", !result.get("ltc_1").sameValue(result.get("ltc_2")));
        output.set("halfWidthIsUniform", result.get("halfWidth").sameValue(node.get("halfWidth")));
        output.set("lightColorIsColorNode", result.get("lightColor").sameValue(node.get("colorNode")));
        output.set("ltc1WrapsFloat", result.get("ltc_1").get("value").sameValue(library.get("LTC_FLOAT_1")));
        output.set("ltc2WrapsFloat", result.get("ltc_2").get("value").sameValue(library.get("LTC_FLOAT_2")));
        output.set("halfLtc1WrapsHalf", halfResult.get("ltc_1").get("value").sameValue(library.get("LTC_HALF_1")));
        output.set("halfLtc2WrapsHalf", halfResult.get("ltc_2").get("value").sameValue(library.get("LTC_HALF_2")));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
