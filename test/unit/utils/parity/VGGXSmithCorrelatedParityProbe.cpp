#include "nodes/functions/BSDF/V_GGX_SmithCorrelated.h"
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
        const auto function = nativeVGGXSmithCorrelated();
        auto inputs = JSValue::object();
        inputs.set("alpha", nativeTSLBaseBinding("float").call({}, {0.5}));
        inputs.set("dotNL", nativeTSLBaseBinding("float").call({}, {0.75}));
        inputs.set("dotNV", nativeTSLBaseBinding("float").call({}, {0.65}));
        const auto result = function.call({}, {inputs});
        auto output = JSValue::object();
        own(output, "name", function.get("name"));
        own(output, "isFn", function.get("isFn"));
        own(output, "source", function.get("shaderNode").get("jsFunc").get("toString").call(function.get("shaderNode").get("jsFunc")));
        own(output, "layoutType", function.get("shaderNode").get("layout").get("type"));
        own(output, "type", result.get("type"));
        own(output, "nodeType", result.get("node").get("type"));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
