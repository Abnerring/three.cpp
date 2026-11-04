#include "nodes/utils/SpriteSheetUV.h"

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
        const auto module = nativeSpriteSheetUVModule();
        const auto fn = nativeSpriteSheetUV();
        const auto count = nativeTSLBaseBinding("vec2").call({}, {6, 6});
        const auto uv = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
        const auto frame = nativeTSLBaseBinding("float").call({}, {7});
        const auto call = fn.call({}, {count, uv, frame});

        auto output = JSValue::object();
        own(output, "fn", describe(fn));
        own(output, "moduleSame", module.get("spritesheetUV").sameValue(fn));
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
