#include "nodes/lighting/LightUtils.h"
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
        const auto module = nativeLightUtilsModule();
        const auto function = nativeGetDistanceAttenuation();
        const auto shaderNode = function.get("shaderNode");
        auto inputs = JSValue::object();
        inputs.set("lightDistance", nativeTSLBaseBinding("float").call({}, {2.0}));
        inputs.set("cutoffDistance", nativeTSLBaseBinding("float").call({}, {10.0}));
        inputs.set("decayExponent", nativeTSLBaseBinding("float").call({}, {2.0}));
        const auto result = function.call({}, {inputs});

        auto output = JSValue::object();
        own(output, "name", function.get("name"));
        own(output, "isFn", function.get("isFn"));
        own(output, "isNode", function.get("isNode"));
        const auto jsFunc = shaderNode.get("jsFunc");
        own(output, "source", jsFunc.get("toString").call(jsFunc));
        own(output, "layout", shaderNode.get("layout"));
        own(output, "moduleSame", module.get("getDistanceAttenuation").sameValue(function));
        own(output, "type", result.get("type"));
        own(output, "nodeType", result.get("node").get("type"));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
