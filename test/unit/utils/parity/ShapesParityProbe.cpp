#include "nodes/shapes/Shapes.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <iostream>

namespace {
using namespace UtilityNodeRuntime;
}

int main() {
    try {
        const auto module = nativeShapesModule();
        const auto shape = nativeShapeCircle();
        const auto coord = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
        const auto call = shape.call({}, {coord});
        auto output = JSValue::object();
        own(output, "type", shape.get("type"));
        own(output, "isFn", shape.get("isFn"));
        own(output, "isNode", shape.get("isNode"));
        const auto length = shape.get("length");
        own(output, "length", length.isCallable() ? JSValue("function") : length);
        own(output, "source", shape.get("shaderNode").get("jsFunc").toString());
        own(output, "moduleSame", module.get("shapeCircle").sameValue(shape));
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
