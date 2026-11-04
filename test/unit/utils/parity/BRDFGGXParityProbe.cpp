#include "nodes/functions/BSDF/BRDF_GGX.h"
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
        auto inputs = JSValue::object();
        inputs.set("lightDirection", nativeTSLBaseBinding("vec3").call({}, {1.0, 0.0, 0.0}));
        inputs.set("f0", nativeTSLBaseBinding("vec3").call({}, {0.04, 0.05, 0.06}));
        inputs.set("f90", nativeTSLBaseBinding("float").call({}, {1.0}));
        inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {0.5}));
        inputs.set("f", nativeTSLBaseBinding("vec3").call({}, {0.2, 0.3, 0.4}));
        const auto function = nativeBRDFGGX();
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
