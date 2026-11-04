#include "nodes/math/UnpackFloatNode.h"
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
    own(result, "isUnpackFloatNode", node.get("isUnpackFloatNode"));
    own(result, "nodeType", node.get("nodeType"));
    return result;
}
}

int main() {
    try {
        const auto uintNode = nativeTSLCoreBinding("uint").call({}, {42});
        auto result = JSValue::object();
        result.set("snorm2", describe(nativeUnpackFloatNodeBinding("unpackSnorm2x16").call({}, {uintNode})));
        result.set("unorm4", describe(nativeUnpackFloatNodeBinding("unpackUnorm4x8").call({}, {uintNode})));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
