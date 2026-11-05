#include "nodes/lighting/SpotLightNode.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"

#include <iostream>

int main() {
    try {
        const auto module = nativeSpotLightNodeModule();
        const auto ctor = nativeSpotLightNodeConstructor();
        // A plain object: the light accessors key a weak map on the light and null is not a key.
        auto light = JSValue::object(nullptr);
        const auto node = ctor.construct({light});
        auto output = JSValue::object(nullptr);
        output.set("moduleSame", module.get("default").sameValue(ctor));
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", node.get("type"));
        output.set("isAnalyticLightNode", node.get("isAnalyticLightNode"));
        for (const char* field : {"coneCosNode", "penumbraCosNode", "cutoffDistanceNode",
                                  "decayExponentNode", "colorNode"}) {
            output.set(std::string(field) + "Type", node.get(field).get("type"));
            output.set(std::string(field) + "Group", node.get(field).get("groupNode").get("name"));
        }
        output.set("getSpotAttenuationLength", node.get("getSpotAttenuation").get("length"));
        output.set("getSpotAttenuationName", node.get("getSpotAttenuation").get("name"));
        output.set("getLightCoordLength", node.get("getLightCoord").get("length"));
        output.set("setupDirectLength", node.get("setupDirect").get("length"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
