#include "nodes/functions/material/getGeometryRoughness.h"
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
        const auto function = nativeGetGeometryRoughness();
        auto geometry = JSValue::object();
        geometry.set("hasAttribute", JSValue::function(
            +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return false; },
            {}, "hasAttribute", 1));
        auto builder = JSValue::object();
        builder.set("geometry", geometry);
        const auto result = function.call({}, {builder});
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
