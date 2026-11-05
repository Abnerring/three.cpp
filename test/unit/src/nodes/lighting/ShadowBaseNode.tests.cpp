#include "nodes/lighting/ShadowBaseNode.h"

#include "nodes/core/Node.h"
// Brings in Args, the argument list type the native functions take.
#include "nodes/lighting/LightingNodeRuntime.h"

#include "catch.hpp"

namespace {
// Args lives in the runtime namespace, so the argument list type is spelled out here.
using ArgList = std::vector<JSValue>;

// A callable that counts its own invocations in a holder object, used for the render object hooks.
JSValue counting(const JSValue& holder, const char* key) {
    return JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto target = captures[0];
            const auto field = captures[1].string();
            target.set(field.c_str(), JSValue(target.get(field.c_str()).toNumber() + 1));
            return {};
        },
        {holder, JSValue(key)}, "", 0);
}

struct Harness {
    JSValue renderer, shadow, scene, counts;
    Harness(double shadowType) {
        counts = JSValue::object(nullptr);
        counts.set("before", 0);
        counts.set("render", 0);
        counts.set("after", 0);
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
    JSValue object(bool castShadow, bool receiveShadow) const {
        auto value = JSValue::object(nullptr);
        value.set("castShadow", castShadow);
        value.set("receiveShadow", receiveShadow);
        value.set("onBeforeShadow", counting(counts, "before"));
        value.set("onAfterShadow", counting(counts, "after"));
        return value;
    }
};
}  // namespace

TEST_CASE("ShadowBaseNode exposes the pinned base shadow shape") {
    auto light = JSValue::object(nullptr);
    const auto module = nativeShadowBaseNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("ShadowBaseNode"));
    // constructor( light ) has no default, so Function.length is 1.
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("ShadowBaseNode"));

    const auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("ShadowBaseNode"));
    REQUIRE(node.get("isShadowBaseNode").strictEquals(true));
    REQUIRE(node.get("light").sameValue(light));
    // Shadows update per render by default.
    REQUIRE(node.get("updateBeforeType").strictEquals("render"));
    REQUIRE(node.get("isCacheable").call(node, {}).strictEquals(false));
    // The module also exports the shadow pass world position property. The node's own type is
    // PropertyNode; the value type it carries is in nodeType.
    const auto property = nativeShadowPositionWorld();
    REQUIRE(property.get("isNode").strictEquals(true));
    REQUIRE(property.get("type").strictEquals("PropertyNode"));
    REQUIRE(property.get("nodeType").strictEquals("vec3"));
}

TEST_CASE("ShadowBaseNode caches one shadow material per light and disposes it") {
    auto light = JSValue::object(nullptr);
    auto otherLight = JSValue::object(nullptr);
    const auto node = nativeShadowBaseNodeConstructor().construct({light});

    const auto material = node.get("getShadowMaterial").call(node, {});
    REQUIRE(material.get("name").strictEquals("ShadowMaterial"));
    REQUIRE(material.get("isShadowPassMaterial").strictEquals(true));
    // NoBlending is nought and fog is off.
    REQUIRE(material.get("blending").toNumber() == Approx(0));
    REQUIRE(material.get("fog").strictEquals(false));
    REQUIRE(material.get("colorNode").get("isNode").strictEquals(true));

    // The library is keyed by light, so the same light answers the same material.
    REQUIRE(node.get("getShadowMaterial").call(node, {}).sameValue(material));
    // A different light gets its own material.
    const auto otherNode = nativeShadowBaseNodeConstructor().construct({otherLight});
    const auto otherMaterial = otherNode.get("getShadowMaterial").call(otherNode, {});
    REQUIRE_FALSE(otherMaterial.sameValue(material));

    // Disposing removes the entry, so the next request builds a fresh material.
    node.get("disposeShadowMaterial").call(node, {});
    const auto rebuilt = node.get("getShadowMaterial").call(node, {});
    REQUIRE_FALSE(rebuilt.sameValue(material));
}

TEST_CASE("ShadowBaseNode render object function honours cast shadow and the VSM receiver rule") {
    auto light = JSValue::object(nullptr);
    const auto node = nativeShadowBaseNodeConstructor().construct({light});
    Harness pcf(1.0);
    const auto fn = node.get("getShadowRenderObjectFunction").call(node, {pcf.renderer, pcf.shadow});
    REQUIRE(fn.isCallable());
    // ( object, scene, _camera, geometry, material, group, lightsNode, clippingContext, passId )
    REQUIRE(fn.get("length").strictEquals(9));
    REQUIRE(fn.get("shadowType").toNumber() == Approx(1));

    // The same renderer and shadow answer the same function, because the library caches them.
    const auto again = node.get("getShadowRenderObjectFunction").call(node, {pcf.renderer, pcf.shadow});
    REQUIRE(again.sameValue(fn));

    // A caster runs the before, render and after steps once.
    fn.call({}, {pcf.object(true, false), pcf.scene, JSValue::object(nullptr),
                 JSValue::object(nullptr), JSValue::object(nullptr), JSValue::object(nullptr),
                 JSValue::object(nullptr), JSValue::object(nullptr), JSValue::object(nullptr)});
    REQUIRE(pcf.counts.get("before").toNumber() == Approx(1));
    REQUIRE(pcf.counts.get("render").toNumber() == Approx(1));
    REQUIRE(pcf.counts.get("after").toNumber() == Approx(1));

    // A non caster under a non VSM type does nothing at all.
    fn.call({}, {pcf.object(false, false), pcf.scene, JSValue::object(nullptr),
                 JSValue::object(nullptr), JSValue::object(nullptr), JSValue::object(nullptr),
                 JSValue::object(nullptr), JSValue::object(nullptr), JSValue::object(nullptr)});
    REQUIRE(pcf.counts.get("before").toNumber() == Approx(1));
    REQUIRE(pcf.counts.get("render").toNumber() == Approx(1));

    // Under VSM a receiver is rendered too, which is the second half of the source condition.
    Harness vsm(3.0);
    const auto vsmFn = node.get("getShadowRenderObjectFunction").call(node, {vsm.renderer, vsm.shadow});
    REQUIRE(vsmFn.get("shadowType").toNumber() == Approx(3));
    REQUIRE_FALSE(vsmFn.sameValue(fn));
    vsmFn.call({}, {vsm.object(false, true), vsm.scene, JSValue::object(nullptr),
                    JSValue::object(nullptr), JSValue::object(nullptr), JSValue::object(nullptr),
                    JSValue::object(nullptr), JSValue::object(nullptr), JSValue::object(nullptr)});
    REQUIRE(vsm.counts.get("before").toNumber() == Approx(1));
    REQUIRE(vsm.counts.get("render").toNumber() == Approx(1));
    REQUIRE(vsm.counts.get("after").toNumber() == Approx(1));
}

TEST_CASE("ShadowBaseNode setupShadowPosition accepts a configuration object") {
    auto light = JSValue::object(nullptr);
    const auto node = nativeShadowBaseNodeConstructor().construct({light});
    auto context = JSValue::object(nullptr);
    auto material = JSValue::object(nullptr);
    auto configuration = JSValue::object(nullptr);
    configuration.set("context", context);
    configuration.set("material", material);
    // With neither override present the source falls back to the world position accessor.
    REQUIRE_NOTHROW(node.get("setupShadowPosition").call(node, {configuration}));
}
