#include "nodes/lighting/LightProbeNode.h"
#include "math/Vector3.h"
#include "math/Color.h"
#include "core/JSValue.h"
#include "core/JSPromise.h"
#include <iostream>

namespace {
void put(JSValue object, const char* key, const JSValue& value) { object.set(key, value); }
JSValue vector(double x, double y, double z) {
    auto value = JSValue::object(nullptr);
    put(value, "isVector3", true); put(value, "x", x); put(value, "y", y); put(value, "z", z);
    return value;
}
}

int main() {
  try {
    auto light = JSValue::object(nullptr);
    put(light, "uuid", "probe"); put(light, "intensity", 2);
    put(light, "color", nativeColorConstructor().construct({.1, .2, .3}));
    auto coefficients = JSValue::array();
    const auto vectorConstructor = nativeVector3Constructor();
    for (int i = 0; i < 9; ++i) {
        auto coefficient = vectorConstructor.construct();
        coefficient.get("set").call(coefficient, {i + 1, i + 2, i + 3});
        coefficients.set(JSValue(i), coefficient);
    }
    auto sh = JSValue::object(nullptr); put(sh, "coefficients", coefficients); put(light, "sh", sh);
    const auto node = nativeLightProbeNodeConstructor().construct({light});
    node.get("update").call(node, {JSValue::object(nullptr)});
    auto out = JSValue::object(nullptr);
    put(out, "type", node.get("type")); put(out, "isLightingNode", node.get("isLightingNode"));
    put(out, "isAnalyticLightNode", node.get("isAnalyticLightNode")); put(out, "arrayLength", node.get("lightProbe").get("array").get("length"));
    put(out, "first", node.get("lightProbe").get("array").get(JSValue(0)));
    put(out, "last", node.get("lightProbe").get("array").get(JSValue(8)));
    std::cout << out.toJSON()->dump() << '\n';
  } catch (const JSThrownValue& error) {
    std::cerr << "JSThrownValue: " << error.value.toJSON()->dump() << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "exception: " << error.what() << '\n';
    return 3;
  }
}
