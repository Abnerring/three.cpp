#include "nodes/functions/BSDF/BRDF_EON.h"
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
        inputs.set("lightDirection", nativeTSLBaseBinding("vec3").call({}, {0.2, 0.7, 0.6}));
        inputs.set("diffuseColor", nativeTSLBaseBinding("vec3").call({}, {0.5, 0.4, 0.3}));
        inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {0.35}));
        inputs.set("normalView", nativeTSLBaseBinding("vec3").call({}, {0.0, 0.0, 1.0}));
        inputs.set("viewDirection", nativeTSLBaseBinding("vec3").call({}, {0.1, 0.2, 0.97}));
        const auto function = nativeBRDFEON();
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
