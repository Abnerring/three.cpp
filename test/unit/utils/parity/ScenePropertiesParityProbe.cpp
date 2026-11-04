#include "nodes/accessors/SceneProperties.h"

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
    own(result, "nodeType", node.get("nodeType"));
    const auto valueType = node.get("value").get("type");
    own(result, "valueType", valueType.isUndefined() ? JSValue(nullptr) : valueType);
    own(result, "value", node.get("value"));
    own(result, "updateType", node.get("updateType"));
    own(result, "group", node.get("groupNode").get("name"));
    own(result, "hasUpdate", node.get("update").isCallable());
    return result;
}
}

int main() {
    try {
        const auto module = nativeScenePropertiesModule();
        auto result = JSValue::object();
        for (const auto* name : {"backgroundBlurriness", "backgroundIntensity", "backgroundRotation"})
            own(result, name, describe(module.get(name)));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

