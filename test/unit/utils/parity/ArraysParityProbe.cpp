#include "nodes/accessors/Arrays.h"

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
    own(result, "isStorageBufferNode", node.get("isStorageBufferNode"));
    own(result, "inputType", node.get("getInputType").call(node));
    own(result, "bufferCount", node.get("bufferCount"));
    own(result, "nodeType", node.get("nodeType"));
    const auto value = node.get("value");
    own(result, "valueStorage", value.get("isStorageBufferAttribute").strictEquals(true) ? "storage" : "instanced");
    own(result, "itemSize", value.get("itemSize"));
    own(result, "count", value.get("count"));
    own(result, "arrayType", value.get("array").get("constructor").get("name"));
    own(result, "arrayLength", value.get("array").get("length"));
    return result;
}
}

int main() {
    try {
        const auto module = nativeArraysModule();
        auto actual = JSValue::object();
        own(actual, "float", describe(module.get("attributeArray").call({}, {5, "float"})));
        own(actual, "vec3", describe(module.get("attributeArray").call({}, {2, "vec3"})));
        own(actual, "instanced", describe(module.get("instancedArray").call({}, {3, "vec4"})));
        std::cout << actual.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

