#include "nodes/functions/BSDF/BRDF_Lambert.h"

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
        const auto function = nativeBRDFLambert();
        auto diffuse = JSValue::object();
        diffuse.set("mul", JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>&) -> JSValue {
                return args.empty() ? JSValue{} : args[0];
            }, {}, "mul", 1));
        auto inputs = JSValue::object();
        inputs.set("diffuseColor", diffuse);
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
