#include "nodes/utils/Oscillators.h"

#include "nodes/tsl/TSLBase.h"

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
    own(result, "isNode", node.get("isNode"));
    return result;
}
}

int main() {
    try {
        nativeOscillatorsModule();
        const auto value = nativeTSLBaseBinding("float").call({}, {.25});
        auto result = JSValue::object();
        for (const auto* name : {"oscSine", "oscSquare", "oscTriangle", "oscSawtooth"})
            own(result, name, describe(nativeOscillatorsBinding(name).call({}, {value})));
        own(result, "default", describe(nativeOscillatorsBinding("oscSine").call()));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
