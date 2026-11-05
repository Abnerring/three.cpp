#include "nodes/functions/BSDF/LTC.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>

int main() {
    try {
        const auto module = nativeLTCModule();
        auto output = JSValue::object(nullptr);
        auto names = JSValue::array();
        auto kinds = JSValue::object(nullptr);
        for (const char* name : {"LTC_Uv", "LTC_Evaluate", "LTC_Evaluate_Volume"}) {
            const auto value = module.get(name);
            names.set(names.length(), JSValue(name));
            kinds.set(name, JSValue(value.isCallable() ? "function" : "other"));
        }
        output.set("names", names);
        output.set("kinds", kinds);

        // LTC_Uv is executed with real nodes, so the returned node can be compared directly.
        auto inputs = JSValue::object(nullptr);
        inputs.set("N", nativeTSLBaseBinding("vec3").call({}, {JSValue(0), JSValue(0), JSValue(1)}));
        inputs.set("V", nativeTSLBaseBinding("vec3").call({}, {JSValue(0), JSValue(0), JSValue(1)}));
        inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {JSValue(0.5)}));
        const auto uv = nativeLTCBinding("LTC_Uv").call({}, {inputs});
        output.set("uvIsNode", uv.get("isNode"));
        auto uvType = uv.get("type");
        output.set("uvType", uvType.isUndefined() ? JSValue("undefined") : JSValue(uvType.string()));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}