#include "nodes/math/TriNoise3D.h"
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
    auto result = JSValue::object();
    own(result, "type", value.get("type"));
    own(result, "isFn", value.get("isFn"));
    own(result, "isNode", value.get("isNode"));
    const auto length = value.get("length");
    own(result, "length", length.isCallable() ? JSValue("function") : length);
    own(result, "name", value.get("name"));
    own(result, "source", value.get("shaderNode").get("jsFunc").toString());
    return result;
}
}

int main() {
    try {
        const auto module = nativeTriNoise3DModule();
        const auto noise = nativeTriNoise3D();
        const auto position = nativeTSLBaseBinding("vec3").call({}, {0, 1, 2});
        const auto speed = nativeTSLBaseBinding("float").call({}, {1});
        const auto time = nativeTSLBaseBinding("float").call({}, {0.5});
        const auto call = noise.call({}, {position, speed, time});
        auto output = JSValue::object();
        own(output, "fn", describe(noise));
        own(output, "moduleSame", module.get("triNoise3D").sameValue(noise));
        own(output, "callType", call.get("type"));
        own(output, "callNodeType", call.get("node").get("type"));
        own(output, "rawInputCount", call.get("node").get("rawInputs").get("length"));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

