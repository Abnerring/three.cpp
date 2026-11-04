#include "nodes/math/Packed4x8IntegerNode.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue describe(const JSValue& value) {
    const auto node = value.get("isPacked4x8IntegerNode").strictEquals(true) ? value : value.get("node");
    auto result = JSValue::object();
    own(result, "type", node.get("type"));
    own(result, "method", node.get("method"));
    own(result, "isPacked4x8IntegerNode", node.get("isPacked4x8IntegerNode"));
    own(result, "inputType", node.get("getInputType").call(node));
    own(result, "nodeType", node.get("generateNodeType").call(node, {JSValue::object()}));
    return result;
}
}

int main() {
    try {
        const auto uintNode = nativeTSLBaseBinding("uint").call({}, {7});
        const auto uvec = nativeTSLBaseBinding("uvec4").call({}, {1, 2, 3, 4});
        const auto ivec = nativeTSLBaseBinding("ivec4").call({}, {-1, 2, -3, 4});
        auto result = JSValue::object();
        result.set("dotU", describe(nativePacked4x8IntegerNodeBinding("dot4U8Packed").call({}, {uintNode, uintNode})));
        result.set("dotI", describe(nativePacked4x8IntegerNodeBinding("dot4I8Packed").call({}, {uintNode, uintNode})));
        result.set("packI", describe(nativePacked4x8IntegerNodeBinding("pack4xI8").call({}, {ivec})));
        result.set("packU", describe(nativePacked4x8IntegerNodeBinding("pack4xU8").call({}, {uvec})));
        result.set("packIC", describe(nativePacked4x8IntegerNodeBinding("pack4xI8Clamp").call({}, {ivec})));
        result.set("packUC", describe(nativePacked4x8IntegerNodeBinding("pack4xU8Clamp").call({}, {uvec})));
        result.set("unpackI", describe(nativePacked4x8IntegerNodeBinding("unpack4xI8").call({}, {uintNode})));
        result.set("unpackU", describe(nativePacked4x8IntegerNodeBinding("unpack4xU8").call({}, {uintNode})));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
