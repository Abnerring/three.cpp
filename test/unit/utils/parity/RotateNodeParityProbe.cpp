#include "nodes/utils/RotateNode.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>

namespace {
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

JSValue nullable(const JSValue& value) {
    return value.isUndefined() ? JSValue(nullptr) : value;
}

JSValue describe(const JSValue& node) {
    auto result = JSValue::object();
    own(result, "type", nullable(node.get("type")));
    own(result, "nodeType", nullable(node.get("nodeType")));
    own(result, "positionType", nullable(node.get("positionNode").get("type")));
    own(result, "rotationType", nullable(node.get("rotationNode").get("type")));
    own(result, "order", nullable(node.get("getOrder").call(node)));
    own(result, "cacheKey", nullable(node.get("customCacheKey").call(node)));
    return result;
}
}

int main() {
    try {
        nativeRotateNodeModule();
        const auto vec2 = nativeTSLBaseBinding("vec2");
        const auto vec3 = nativeTSLBaseBinding("vec3");
        const auto floatNode = nativeTSLBaseBinding("float");
        const auto position2 = vec2.call({}, {.25, .5});
        const auto angle = floatNode.call({}, {.75});
        const auto position3 = vec3.call({}, {.25, .5, .75});
        const auto rotation3 = vec3.call({}, {.1, .2, .3});
        auto result = JSValue::object();
        own(result, "constructor2", describe(nativeRotateNodeConstructor().construct({position2, angle})));
        own(result, "factory2", describe(nativeRotateNodeBinding("rotate").call({}, {position2, angle})));
        own(result, "constructor3", describe(nativeRotateNodeConstructor().construct({position3, rotation3, "YZX"})));
        auto changed = nativeRotateNodeBinding("rotate").call({}, {position3, rotation3});
        changed.get("setOrder").call(changed, {"ZXY"});
        own(result, "changed", describe(changed));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
