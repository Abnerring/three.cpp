#include "nodes/gpgpu/BarrierNode.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

JSValue describeNode(const JSValue& node) {
    auto result = JSValue::object();
    own(result, "type", node.get("type"));
    own(result, "scope", node.get("scope"));
    own(result, "isBarrierNode", node.get("isBarrierNode"));
    own(result, "cacheable", node.get("isCacheable").call(node));
    auto methods = JSValue::object();
    for (const auto* name : {"isCacheable", "setup", "generate"})
        own(methods, name, node.get(name).get("length"));
    own(result, "methods", methods);
    return result;
}

JSValue describeFunction(const JSValue& function) {
    auto result = JSValue::object();
    own(result, "length", function.get("length"));
    own(result, "source", function.nativeFunctionSource());
    return result;
}
}

int main() {
    try {
        const auto module = nativeBarrierNodeModule();
        auto result = JSValue::object();
        own(result, "constructorName", module.get("default").get("name"));
        own(result, "constructorLength", module.get("default").get("length"));
        for (const auto* scope : {"workgroup", "storage", "texture"})
            own(result, scope, describeNode(module.get("barrier").call({}, {scope})));
        for (const auto* name : {"workgroupBarrier", "storageBarrier", "textureBarrier"})
            own(result, name, describeFunction(module.get(name)));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

