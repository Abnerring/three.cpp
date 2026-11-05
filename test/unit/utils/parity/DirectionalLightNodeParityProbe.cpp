#include "nodes/lighting/DirectionalLightNode.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"

#include <iostream>

int main() {
    try {
        const auto module = nativeDirectionalLightNodeModule();
        const auto ctor = nativeDirectionalLightNodeConstructor();
        const auto node = ctor.construct({});
        auto output = JSValue::object(nullptr);
        output.set("moduleSame", module.get("default").sameValue(ctor));
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", node.get("type"));
        output.set("isAnalyticLightNode", node.get("isAnalyticLightNode"));
        output.set("lightNull", node.get("light").isNull());
        output.set("setupDirectLength", node.get("setupDirect").get("length"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
