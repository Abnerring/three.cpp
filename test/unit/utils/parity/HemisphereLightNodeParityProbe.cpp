#include "nodes/lighting/HemisphereLightNode.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"

#include <iostream>

int main() {
    try {
        const auto module = nativeHemisphereLightNodeModule();
        const auto ctor = nativeHemisphereLightNodeConstructor();
        // A plain object rather than the default: the light accessors key a weak map on the light
        // and null is not a valid key.
        auto light = JSValue::object(nullptr);
        const auto node = ctor.construct({light});
        auto output = JSValue::object(nullptr);
        output.set("moduleSame", module.get("default").sameValue(ctor));
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", node.get("type"));
        output.set("isAnalyticLightNode", node.get("isAnalyticLightNode"));
        output.set("lightIsGiven", node.get("light").sameValue(light));
        output.set("positionNodeType", node.get("lightPositionNode").get("type"));
        output.set("positionNodeGroup", node.get("lightPositionNode").get("groupNode").get("name"));
        output.set("directionIsNode", node.get("lightDirectionNode").get("isNode"));
        output.set("groundNodeType", node.get("groundColorNode").get("type"));
        output.set("groundNodeGroup", node.get("groundColorNode").get("groupNode").get("name"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
