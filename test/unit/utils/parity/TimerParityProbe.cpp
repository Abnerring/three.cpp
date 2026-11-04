#include "nodes/utils/Timer.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

JSValue describe(const JSValue& node) {
    auto value = JSValue::object();
    own(value, "type", node.get("type"));
    own(value, "isNode", node.get("isNode"));
    own(value, "isUniformNode", node.get("isUniformNode"));
    own(value, "nodeType", node.get("nodeType"));
    own(value, "value", node.get("value"));
    own(value, "groupType", node.get("groupNode").get("type"));
    own(value, "groupName", node.get("groupNode").get("name"));
    own(value, "updateType", node.get("updateType"));
    return value;
}
}

int main() {
    try {
        auto output = JSValue::object();
        own(output, "time", describe(nativeTimerTime()));
        own(output, "deltaTime", describe(nativeTimerDeltaTime()));
        own(output, "frameId", describe(nativeTimerFrameId()));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
