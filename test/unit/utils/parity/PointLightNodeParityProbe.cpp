#include "nodes/lighting/PointLightNode.h"

#include "core/JSValue.h"
#include "math/Color.h"
#include "nodes/core/Node.h"

#include <iostream>

int main() {
    try {
        const auto module = nativePointLightNodeModule();
        const auto ctor = nativePointLightNodeConstructor();
        const auto factory = nativeDirectPointLight();

        auto light = JSValue::object(nullptr);
        auto color = nativeColorConstructor().construct();
        color.set("r", 0.25); color.set("g", 0.5); color.set("b", 0.75);
        light.set("color", color);
        light.set("intensity", 2);
        light.set("distance", 7);
        light.set("decay", 1.5);

        const auto node = ctor.construct({light});
        const auto cutoffBefore = node.get("cutoffDistanceNode").get("value");
        const auto decayBefore = node.get("decayExponentNode").get("value");
        node.get("update").call(node, {JSValue::object(nullptr)});

        auto output = JSValue::object(nullptr);
        output.set("moduleSame", module.get("default").sameValue(ctor));
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", node.get("type"));
        output.set("isAnalyticLightNode", node.get("isAnalyticLightNode"));
        output.set("cutoffBefore", cutoffBefore);
        output.set("decayBefore", decayBefore);
        output.set("cutoffAfter", node.get("cutoffDistanceNode").get("value"));
        output.set("decayAfter", node.get("decayExponentNode").get("value"));
        output.set("colorR", node.get("color").get("r"));
        output.set("colorG", node.get("color").get("g"));
        output.set("colorB", node.get("color").get("b"));
        output.set("factoryName", factory.get("name"));
        output.set("factoryLength", factory.get("length"));
        output.set("setupDirectLength", node.get("setupDirect").get("length"));
        output.set("updateLength", node.get("update").get("length"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
