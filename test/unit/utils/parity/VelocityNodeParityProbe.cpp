#include "nodes/accessors/VelocityNode.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; object.defineProperty(key, d);
}
JSValue describe(const JSValue& node) {
    auto out = JSValue::object();
    own(out, "type", node.get("type"));
    own(out, "nodeType", node.get("nodeType"));
    own(out, "projectionMatrix", node.get("projectionMatrix"));
    own(out, "updateType", node.get("updateType"));
    own(out, "updateAfterType", node.get("updateAfterType"));
    auto uniforms = JSValue::object();
    for (const auto* key : {"previousModelWorldMatrix", "currentProjectionMatrix", "previousProjectionMatrix", "previousCameraViewMatrix"}) {
        auto value = node.get(key), entry = JSValue::object();
        own(entry, "type", value.get("type"));
        own(entry, "group", value.get("groupNode").get("name"));
        own(uniforms, key, entry);
    }
    own(out, "uniforms", uniforms);
    return out;
}
}

int main() {
    try {
        nativeVelocityNodeModule();
        auto result = JSValue::object();
        own(result, "node", describe(nativeVelocityNodeConstructor().construct()));
        own(result, "velocity", describe(nativeVelocityNodeBinding("velocity")));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
