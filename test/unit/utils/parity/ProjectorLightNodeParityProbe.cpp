#include "nodes/lighting/ProjectorLightNode.h"

#include "core/JSValue.h"
#include "math/Color.h"
#include "nodes/core/Node.h"

#include <iostream>

namespace {
JSValue makeLight(double angle, double penumbra) {
    auto light = JSValue::object(nullptr);
    auto color = nativeColorConstructor().construct();
    color.set("r", 0.25); color.set("g", 0.5); color.set("b", 0.75);
    light.set("color", color);
    light.set("intensity", 2);
    light.set("angle", angle);
    light.set("penumbra", penumbra);
    light.set("aspect", nullptr);
    light.set("map", nullptr);
    light.set("shadow", JSValue::object(nullptr));
    return light;
}
}  // namespace

int main() {
    try {
        const auto module = nativeProjectorLightNodeModule();
        const auto ctor = nativeProjectorLightNodeConstructor();
        auto light = makeLight(0.5, 0.25);
        const auto node = ctor.construct({light});
        node.get("update").call(node, {JSValue::object(nullptr)});

        auto output = JSValue::object(nullptr);
        output.set("moduleSame", module.get("default").sameValue(ctor));
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", node.get("type"));
        output.set("isAnalyticLightNode", node.get("isAnalyticLightNode"));
        output.set("coneCosNodeValue", node.get("coneCosNode").get("value"));
        output.set("penumbraCosNodeValue", node.get("penumbraCosNode").get("value"));
        output.set("colorR", node.get("color").get("r"));
        output.set("shadowAspect", light.get("shadow").get("aspect"));
        output.set("getSpotAttenuationLength", node.get("getSpotAttenuation").get("length"));
        output.set("updateLength", node.get("update").get("length"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
