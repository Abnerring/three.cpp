#include "nodes/functions/material/getParallaxCorrectNormal.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
}

int main() {
    try {
        auto inputs = JSValue::array();
        inputs.set(JSValue(0), nativeTSLBaseBinding("vec3").call({}, {1.0, 2.0, 3.0}));
        inputs.set(JSValue(1), nativeTSLBaseBinding("vec3").call({}, {10.0, 20.0, 30.0}));
        inputs.set(JSValue(2), nativeTSLBaseBinding("vec3").call({}, {0.0, 1.0, 2.0}));
        const auto function = nativeGetParallaxCorrectNormal();
        const auto result = function.call({}, {inputs});
        auto output = JSValue::object();
        own(output, "name", function.get("name"));
        own(output, "isFn", function.get("isFn"));
        own(output, "source", function.get("shaderNode").get("jsFunc").get("toString").call(function.get("shaderNode").get("jsFunc")));
        own(output, "type", result.get("type"));
        own(output, "nodeType", result.get("node").get("type"));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
