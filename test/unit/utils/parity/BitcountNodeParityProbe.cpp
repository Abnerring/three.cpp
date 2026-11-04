#include "nodes/math/BitcountNode.h"
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
    const auto node = value.get("isBitcountNode").strictEquals(true) ? value : value.get("node");
    auto result = JSValue::object();
    own(result, "type", node.get("type"));
    own(result, "method", node.get("method"));
    own(result, "isBitcountNode", node.get("isBitcountNode"));
    return result;
}
}

int main() {
    try {
        const auto value = nativeTSLBaseBinding("uint").call({}, {7});
        auto result = JSValue::object();
        result.set("trailing", describe(nativeBitcountNodeBinding("countTrailingZeros").call({}, {value})));
        result.set("leading", describe(nativeBitcountNodeBinding("countLeadingZeros").call({}, {value})));
        result.set("one", describe(nativeBitcountNodeBinding("countOneBits").call({}, {value})));
        std::cout << result.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
