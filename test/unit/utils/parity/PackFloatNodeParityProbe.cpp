#include "nodes/math/PackFloatNode.h"
#include "nodes/tsl/TSLCore.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue describe(const JSValue& node) {
    auto result = JSValue::object();
    own(result, "type", node.get("type"));
    own(result, "encoding", node.get("encoding"));
    own(result, "layout", node.get("layout"));
    own(result, "isPackFloatNode", node.get("isPackFloatNode"));
    own(result, "nodeType", node.get("nodeType"));
    return result;
}
}

int main() {
    try {
        const auto vec2 = nativeTSLCoreBinding("vec2").call({}, {0.1, 0.2});
        const auto vec4 = nativeTSLCoreBinding("vec4").call({}, {0.1, 0.2, 0.3, 0.4});
        auto result = JSValue::object();
        result.set("snorm2", describe(nativePackFloatNodeBinding("packSnorm2x16").call({}, {vec2})));
        result.set("unorm4", describe(nativePackFloatNodeBinding("packUnorm4x8").call({}, {vec4})));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
