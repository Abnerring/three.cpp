#include "nodes/utils/MatcapUV.h"

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
        const auto module = nativeMatcapUVModule();
        const auto node = nativeMatcapUV();
        auto output = JSValue::object();
        own(output, "type", node.get("type"));
        own(output, "nodeType", node.get("node").get("type"));
        own(output, "moduleType", module.get("matcapUV").get("type"));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
