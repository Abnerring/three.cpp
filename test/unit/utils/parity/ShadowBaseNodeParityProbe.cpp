#include "nodes/lighting/ShadowBaseNode.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"
// Brings in Args, the argument list type the native functions take.
#include "nodes/lighting/LightingNodeRuntime.h"

#include <iostream>

namespace {
// Args lives in the runtime namespace, so the argument list type is spelled out here.
using ArgList = std::vector<JSValue>;

JSValue counting(const JSValue& holder, const char* key) {
    return JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto target = captures[0];
            const auto name = captures[1].string();
            target.set(name.c_str(), JSValue(target.get(name.c_str()).toNumber() + 1));
            return {};
        },
        {holder, JSValue(key)}, "", 0);
}
struct Harness {
    JSValue renderer, shadow, scene;
    explicit Harness(const JSValue& counts, double shadowType) {
        auto shadowMap = JSValue::object(nullptr);
        shadowMap.set("type", shadowType);
        renderer = JSValue::object(nullptr);
        renderer.set("shadowMap", shadowMap);
        renderer.set("renderObject", counting(counts, "render"));
        shadow = JSValue::object(nullptr);
        shadow.set("camera", JSValue::object(nullptr));
        scene = JSValue::object(nullptr);
        scene.set("overrideMaterial", nullptr);
    }
    JSValue object(const JSValue& counts, bool castShadow, bool receiveShadow) const {
        auto value = JSValue::object(nullptr);
        value.set("castShadow", castShadow);
        value.set("receiveShadow", receiveShadow);
        value.set("onBeforeShadow", counting(counts, "before"));
        value.set("onAfterShadow", counting(counts, "after"));
        return value;
    }
};
JSValue empty() { return JSValue::object(nullptr); }
}  // namespace

int main() {
    try {
        auto light = JSValue::object(nullptr);
        auto otherLight = JSValue::object(nullptr);
        const auto node = nativeShadowBaseNodeConstructor().construct({light});
        const auto property = nativeShadowPositionWorld();

        const auto material = node.get("getShadowMaterial").call(node, {});
        const auto cachedMaterial = node.get("getShadowMaterial").call(node, {});
        const auto otherNode = nativeShadowBaseNodeConstructor().construct({otherLight});
        const auto otherMaterial = otherNode.get("getShadowMaterial").call(otherNode, {});
        node.get("disposeShadowMaterial").call(node, {});
        const auto rebuiltMaterial = node.get("getShadowMaterial").call(node, {});

        auto counts = JSValue::object(nullptr);
        counts.set("before", 0);
        counts.set("render", 0);
        counts.set("after", 0);
        Harness pcf(counts, 1.0);
        const auto fn =
            node.get("getShadowRenderObjectFunction").call(node, {pcf.renderer, pcf.shadow});
        const auto cachedFn =
            node.get("getShadowRenderObjectFunction").call(node, {pcf.renderer, pcf.shadow});

        const auto callWith = [&](const JSValue& callable, const JSValue& target) {
            callable.call({}, {target, pcf.scene, empty(), empty(), empty(), empty(), empty(), empty(),
                               empty()});
        };
        callWith(fn, pcf.object(counts, true, false));
        auto afterCaster = JSValue::object(nullptr);
        afterCaster.set("before", counts.get("before"));
        afterCaster.set("render", counts.get("render"));
        afterCaster.set("after", counts.get("after"));
        callWith(fn, pcf.object(counts, false, false));
        auto afterNonCaster = JSValue::object(nullptr);
        afterNonCaster.set("before", counts.get("before"));
        afterNonCaster.set("render", counts.get("render"));

        auto vsmCounts = JSValue::object(nullptr);
        vsmCounts.set("before", 0);
        vsmCounts.set("render", 0);
        vsmCounts.set("after", 0);
        Harness vsm(vsmCounts, 3.0);
        const auto vsmFn =
            node.get("getShadowRenderObjectFunction").call(node, {vsm.renderer, vsm.shadow});
        vsmFn.call({}, {vsm.object(vsmCounts, false, true), vsm.scene, empty(), empty(), empty(),
                        empty(), empty(), empty(), empty()});
        auto afterVsm = JSValue::object(nullptr);
        afterVsm.set("before", vsmCounts.get("before"));
        afterVsm.set("render", vsmCounts.get("render"));
        afterVsm.set("after", vsmCounts.get("after"));

        auto output = JSValue::object(nullptr);
        output.set("name", nativeShadowBaseNodeBinding("default").get("name"));
        output.set("length", nativeShadowBaseNodeBinding("default").get("length"));
        output.set("staticType", nativeShadowBaseNodeBinding("default").get("type"));
        output.set("instanceType", node.get("type"));
        output.set("isShadowBaseNode", node.get("isShadowBaseNode"));
        output.set("updateBeforeType", node.get("updateBeforeType"));
        output.set("isCacheable", node.get("isCacheable").call(node, {}));
        output.set("propertyType", property.get("type"));
        output.set("propertyIsNode", property.get("isNode"));
        output.set("materialName", material.get("name"));
        output.set("materialIsShadowPass", material.get("isShadowPassMaterial"));
        output.set("materialBlending", material.get("blending"));
        output.set("materialFog", material.get("fog"));
        output.set("materialColorNodeIsNode", material.get("colorNode").get("isNode"));
        output.set("materialCached", cachedMaterial.sameValue(material));
        output.set("otherLightDistinct", !otherMaterial.sameValue(material));
        output.set("rebuiltAfterDispose", !rebuiltMaterial.sameValue(material));
        output.set("fnLength", fn.get("length"));
        output.set("fnShadowType", fn.get("shadowType"));
        output.set("fnCached", cachedFn.sameValue(fn));
        output.set("afterCaster", afterCaster);
        output.set("afterNonCaster", afterNonCaster);
        output.set("vsmShadowType", vsmFn.get("shadowType"));
        output.set("vsmDistinct", !vsmFn.sameValue(fn));
        output.set("afterVsm", afterVsm);
        output.set("source", nativeShadowBaseNodeBinding("default").get("toString").call(
                                 nativeShadowBaseNodeBinding("default")));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
