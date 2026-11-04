#include "nodes/math/BitcastNode.h"
#include "nodes/tsl/TSLBase.h"

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
        const auto module = nativeBitcastNodeModule();
        const auto value = nativeTSLBaseBinding("float").call({}, {1.5});
        const auto direct = module.get("floatBitsToInt").call({}, {value});
        const auto wrapped = module.get("bitcast").call({}, {value, "uint"});
        const auto wrappedNode = wrapped.get("node");

        auto exports = JSValue::object();
        for (const auto* name : {"bitcast", "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat"}) {
            auto item = JSValue::object();
            own(item, "name", module.get(name).get("name"));
            own(item, "length", module.get(name).get("length"));
            own(exports, name, item);
        }

        auto output = JSValue::object();
        own(output, "ctorName", module.get("default").get("name"));
        own(output, "ctorLength", module.get("default").get("length"));
        own(output, "ctorType", module.get("default").get("type"));
        own(output, "exports", exports);

        auto bitcast = JSValue::object();
        own(bitcast, "type", wrapped.get("type"));
        own(bitcast, "nodeType", wrappedNode.get("type"));
        own(bitcast, "nodeConversion", wrappedNode.get("conversionType"));
        own(bitcast, "nodeInput", wrappedNode.get("inputType"));
        own(bitcast, "nodeFlag", wrappedNode.get("isBitcastNode"));
        own(output, "bitcast", bitcast);

        auto directData = JSValue::object();
        own(directData, "type", direct.get("type"));
        own(directData, "conversion", direct.get("conversionType"));
        own(directData, "input", direct.get("inputType"));
        own(directData, "flag", direct.get("isBitcastNode"));
        own(directData, "valueType", direct.get("valueNode").get("type"));
        own(output, "direct", directData);

        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
