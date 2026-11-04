#include "nodes/accessors/Lights.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <iostream>

namespace {
using namespace UtilityNodeRuntime;

void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
}

int main() {
    try {
        const auto module = nativeLightsModule();
        auto output = JSValue::object();
        auto names = module.ownPropertyNames();
        auto keys = JSValue::array();
        for (const auto& name : names) invoke(keys, "push")({name});
        own(output, "keys", keys);
        auto functions = JSValue::object();
        auto light = JSValue::object(nullptr);
        for (const auto* name : {"lightPosition", "lightProjectionUV", "lightShadowMatrix",
                                 "lightTargetDirection", "lightTargetPosition", "lightViewPosition"}) {
            const auto fn = module.get(name);
            auto info = JSValue::object();
            own(info, "name", fn.get("name"));
            own(info, "length", fn.get("length"));
            own(info, "resultType", fn.call({}, {light}).get("type"));
            own(functions, name, info);
        }
        own(output, "functions", functions);
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
