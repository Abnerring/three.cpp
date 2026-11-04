#include "nodes/procedural/Checker.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <iostream>

namespace {
using namespace UtilityNodeRuntime;
}

int main() {
    try {
        const auto module = nativeCheckerModule();
        const auto checker = nativeChecker();
        const auto coord = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
        const auto call = checker.call({}, {coord});
        auto output = JSValue::object();
        own(output, "type", checker.get("type"));
        own(output, "isFn", checker.get("isFn"));
        own(output, "isNode", checker.get("isNode"));
        const auto length = checker.get("length");
        own(output, "length", length.isCallable() ? JSValue("function") : length);
        own(output, "source", checker.get("shaderNode").get("jsFunc").toString());
        own(output, "moduleSame", module.get("checker").sameValue(checker));
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
