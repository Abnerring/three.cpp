#include "nodes/lighting/BasicEnvironmentNode.h"
#include "nodes/core/Node.h"
#include "core/JSValue.h"

#include <iostream>

int main() {
    try {
        const auto module = nativeBasicEnvironmentNodeModule();
        const auto ctor = nativeBasicEnvironmentNodeConstructor();
        const auto node = ctor.construct({});
        auto output = JSValue::object(nullptr);
        output.set("moduleSame", module.get("default").sameValue(ctor));
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("type", node.get("type"));
        output.set("isLightingNode", node.get("isLightingNode"));
        output.set("envNodeNull", node.get("envNode").isNull());
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
