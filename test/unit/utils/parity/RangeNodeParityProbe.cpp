#include "nodes/geometry/RangeNode.h"
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
    own(result, "salt", value.get("_salt"));
    own(result, "isCacheable", value.get("isCacheable").call(value));
    own(result, "minType", value.get("minNode").get("type"));
    own(result, "maxType", value.get("maxNode").get("type"));
    return result;
}
}

int main() {
    try {
        auto result = JSValue::object();
        result.set("default", describe(nativeRangeNodeBinding("range").call({}, {})));
        const auto minNode = nativeTSLBaseBinding("float").call({}, {1});
        const auto maxNode = nativeTSLBaseBinding("float").call({}, {2});
        result.set("explicit", describe(nativeRangeNodeBinding("range").call({}, {minNode, maxNode})));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
