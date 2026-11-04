#include "nodes/utils/ViewportUtils.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <iostream>

int main() {
    try {
        const auto module = nativeViewportUtilsModule();
        const auto value = nativeViewportSafeUV();
        const auto coord = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
        const auto call = value.call({}, {coord});
        auto output = JSValue::object();
        UtilityNodeRuntime::own(output, "type", value.get("type"));
        UtilityNodeRuntime::own(output, "isFn", value.get("isFn"));
        UtilityNodeRuntime::own(output, "isNode", value.get("isNode"));
        const auto length = value.get("length");
        UtilityNodeRuntime::own(output, "length", length.isCallable() ? JSValue("function") : length);
        UtilityNodeRuntime::own(output, "source", value.get("shaderNode").get("jsFunc").toString());
        UtilityNodeRuntime::own(output, "moduleSame", module.get("viewportSafeUV").sameValue(value));
        UtilityNodeRuntime::own(output, "callType", call.get("type"));
        UtilityNodeRuntime::own(output, "callNodeType", call.get("node").get("type"));
        UtilityNodeRuntime::own(output, "rawInputCount", call.get("node").get("rawInputs").get("length"));
        std::cout << output.toJSON()->dump();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
