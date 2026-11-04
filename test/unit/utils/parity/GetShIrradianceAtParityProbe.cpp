#include "nodes/functions/material/getShIrradianceAt.h"
#include "nodes/accessors/UniformArrayNode.h"
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
        auto coefficients = JSValue::array();
        for (int i = 0; i < 9; ++i)
            coefficients.set(JSValue(i), nativeTSLBaseBinding("vec3").call({}, {i + 1.0, i + 2.0, i + 3.0}));
        UniformArrayNode shCoefficients(coefficients, "vec3");
        auto inputs = JSValue::array();
        inputs.set(JSValue(0), nativeTSLBaseBinding("vec3").call({}, {1.0, 0.0, 0.0}));
        inputs.set(JSValue(1), static_cast<JSValue>(shCoefficients));
        const auto function = nativeGetShIrradianceAt();
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
