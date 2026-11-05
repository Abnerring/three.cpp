#include "nodes/lighting/EnvironmentNode.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue countingAssign(const JSValue& holder) {
    return JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto target = captures[0];
            target.set("count", JSValue(target.get("count").toNumber() + 1));
            return {};
        },
        {holder}, "addAssign", 1);
}
}  // namespace

int main() {
    try {
        const auto module = nativeEnvironmentNodeModule();
        const auto ctor = nativeEnvironmentNodeConstructor();

        // Builder stand-in, matching what the differential builds upstream.
        auto contexts = JSValue::array();
        auto state = JSValue::object(nullptr);
        state.set("contexts", contexts);
        auto envNode = nativeTSLBaseBinding("vec3").call({}, {JSValue(1), JSValue(1), JSValue(1)});
        envNode.set("context", JSValue::function(
            +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
                auto store = captures[0];
                auto list = store.get("contexts");
                list.set(list.length(), args.size() > 0 ? args[0] : JSValue{});
                return captures[1];
            },
            {state, envNode}, "context", 1));

        auto radiance = JSValue::object(nullptr);
        radiance.set("count", 0);
        radiance.set("addAssign", countingAssign(radiance));
        auto irradiance = JSValue::object(nullptr);
        irradiance.set("count", 0);
        irradiance.set("addAssign", countingAssign(irradiance));
        auto lightingModel = JSValue::object(nullptr);
        auto context = JSValue::object(nullptr);
        context.set("radiance", radiance);
        context.set("iblIrradiance", irradiance);
        context.set("lightingModel", lightingModel);
        auto material = JSValue::object(nullptr);
        material.set("useAnisotropy", false);
        material.set("anisotropy", 0);
        material.set("useRetroreflection", false);
        material.set("retroreflectivity", 0);
        auto renderer = JSValue::object(nullptr);
        auto builder = JSValue::object(nullptr);
        builder.set("material", material);
        builder.set("context", context);
        builder.set("renderer", renderer);

        const auto node = ctor.construct({envNode});
        node.get("setup").call(node, {builder});

        const auto radianceContext = contexts.get("0");
        const auto firstUv = radianceContext.get("getUV").call({}, {});
        const auto secondUv = radianceContext.get("getUV").call({}, {});
        const auto irradianceContext = contexts.get("1");

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("instanceType", node.get("type"));
        output.set("isLightingNode", node.get("isLightingNode"));
        output.set("setupLength", node.get("setup").get("length"));
        output.set("cacheLength", node.get("_getPMREMNodeCache").get("length"));
        output.set("radianceAssigns", radiance.get("count"));
        output.set("irradianceAssigns", irradiance.get("count"));
        output.set("contextCount", contexts.length());
        output.set("getUvIsNode", firstUv.get("isNode"));
        output.set("getUvStable", firstUv.sameValue(secondUv));
        output.set("radianceLevelIsNode",
                   radianceContext.get("getTextureLevel").call({}, {}).get("isNode"));
        output.set("irradianceLevelIsNode",
                   irradianceContext.get("getTextureLevel").call({}, {}).get("isNode"));
        // The per renderer cache answers the same map twice and a different one for another renderer.
        auto otherRenderer = JSValue::object(nullptr);
        const auto cacheA = node.get("_getPMREMNodeCache").call(node, {renderer});
        const auto cacheB = node.get("_getPMREMNodeCache").call(node, {renderer});
        const auto cacheC = node.get("_getPMREMNodeCache").call(node, {otherRenderer});
        output.set("cacheStable", cacheA.sameValue(cacheB));
        output.set("cachePerRenderer", !cacheA.sameValue(cacheC));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
