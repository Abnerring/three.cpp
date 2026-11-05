#include "nodes/lighting/EnvironmentNode.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

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

// A builder stand-in that records the contexts the environment node asks for, so the lazily built
// radiance and irradiance contexts can be inspected afterwards.
struct Harness {
    JSValue envNode, contexts, builder, radiance, irradiance, clearcoat, material, renderer;

    Harness(bool retroreflection, bool withClearcoat) {
        contexts = JSValue::array();
        auto state = JSValue::object(nullptr);
        state.set("contexts", contexts);
        envNode = nativeTSLBaseBinding("vec3").call({}, {JSValue(1), JSValue(1), JSValue(1)});
        envNode.set("context", JSValue::function(
            +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
                auto store = captures[0];
                auto list = store.get("contexts");
                list.set(list.length(), args.size() > 0 ? args[0] : JSValue{});
                return captures[1];
            },
            {state, envNode}, "context", 1));

        radiance = JSValue::object(nullptr);
        radiance.set("count", 0);
        radiance.set("addAssign", countingAssign(radiance));
        irradiance = JSValue::object(nullptr);
        irradiance.set("count", 0);
        irradiance.set("addAssign", countingAssign(irradiance));

        auto lightingModel = JSValue::object(nullptr);
        if (withClearcoat) {
            clearcoat = JSValue::object(nullptr);
            clearcoat.set("count", 0);
            clearcoat.set("addAssign", countingAssign(clearcoat));
            lightingModel.set("clearcoatRadiance", clearcoat);
        }

        auto context = JSValue::object(nullptr);
        context.set("radiance", radiance);
        context.set("iblIrradiance", irradiance);
        context.set("lightingModel", lightingModel);

        material = JSValue::object(nullptr);
        material.set("useAnisotropy", false);
        material.set("anisotropy", 0);
        material.set("useRetroreflection", retroreflection);
        material.set("retroreflectivity", 0);

        renderer = JSValue::object(nullptr);
        builder = JSValue::object(nullptr);
        builder.set("material", material);
        builder.set("context", context);
        builder.set("renderer", renderer);
    }
};
}  // namespace

TEST_CASE("EnvironmentNode exposes the pinned environment shape") {
    const auto module = nativeEnvironmentNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("EnvironmentNode"));
    // constructor( envNode = null ) has a default, so Function.length is 0.
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("EnvironmentNode"));

    auto envNode = JSValue::object(nullptr);
    const auto node = ctor.construct({envNode});
    REQUIRE(node.get("type").strictEquals("EnvironmentNode"));
    REQUIRE(node.get("isLightingNode").strictEquals(true));
    REQUIRE(node.get("envNode").sameValue(envNode));
    REQUIRE(node.get("setup").get("length").strictEquals(1));
    REQUIRE(node.get("_getPMREMNodeCache").get("length").strictEquals(1));

    // The default environment node is nothing at all.
    const auto bare = ctor.construct();
    REQUIRE(bare.get("envNode").isNull());
}

TEST_CASE("EnvironmentNode caches one PMREM node map per renderer") {
    const auto node = nativeEnvironmentNodeConstructor().construct();
    auto renderer = JSValue::object(nullptr);
    auto otherRenderer = JSValue::object(nullptr);
    const auto first = node.get("_getPMREMNodeCache").call(node, {renderer});
    const auto again = node.get("_getPMREMNodeCache").call(node, {renderer});
    const auto other = node.get("_getPMREMNodeCache").call(node, {otherRenderer});
    // The renderer cache is keyed by renderer and each entry is its own map.
    REQUIRE(first.sameValue(again));
    REQUIRE_FALSE(first.sameValue(other));
}

TEST_CASE("EnvironmentNode setup assigns radiance and irradiance from the environment") {
    Harness harness(false, false);
    const auto node = nativeEnvironmentNodeConstructor().construct({harness.envNode});
    node.get("setup").call(node, {harness.builder});

    // One radiance term and one irradiance term, each assigned once.
    REQUIRE(harness.radiance.get("count").toNumber() == Approx(1));
    REQUIRE(harness.irradiance.get("count").toNumber() == Approx(1));
    // Without retroreflection and without a clearcoat radiance there are exactly two contexts.
    REQUIRE(harness.contexts.length() == Approx(2));

    const auto radianceContext = harness.contexts.get("0");
    REQUIRE(radianceContext.get("getUV").isCallable());
    REQUIRE(radianceContext.get("getTextureLevel").isCallable());
    // The reflection vector is built once and then reused, which is what the closure does.
    const auto firstUv = radianceContext.get("getUV").call({}, {});
    const auto secondUv = radianceContext.get("getUV").call({}, {});
    REQUIRE(firstUv.get("isNode").strictEquals(true));
    REQUIRE(firstUv.sameValue(secondUv));
    // The texture level for radiance is the roughness node itself.
    REQUIRE(radianceContext.get("getTextureLevel").call({}, {}).get("isNode").strictEquals(true));

    const auto irradianceContext = harness.contexts.get("1");
    REQUIRE(irradianceContext.get("getTextureLevel").call({}, {}).get("isNode").strictEquals(true));
}

TEST_CASE("EnvironmentNode setup adds a clearcoat radiance term when the model has one") {
    Harness harness(false, true);
    const auto node = nativeEnvironmentNodeConstructor().construct({harness.envNode});
    node.get("setup").call(node, {harness.builder});
    REQUIRE(harness.radiance.get("count").toNumber() == Approx(1));
    REQUIRE(harness.irradiance.get("count").toNumber() == Approx(1));
    REQUIRE(harness.clearcoat.get("count").toNumber() == Approx(1));
    // Radiance, irradiance and the clearcoat context.
    REQUIRE(harness.contexts.length() == Approx(3));
}

TEST_CASE("EnvironmentNode setup asks for a retro radiance context when retroreflection is on") {
    Harness harness(true, false);
    const auto node = nativeEnvironmentNodeConstructor().construct({harness.envNode});
    node.get("setup").call(node, {harness.builder});
    REQUIRE(harness.radiance.get("count").toNumber() == Approx(1));
    REQUIRE(harness.irradiance.get("count").toNumber() == Approx(1));
    // Radiance, retro radiance and irradiance.
    REQUIRE(harness.contexts.length() == Approx(3));

    // The retro context builds its own vector, lazily, and keeps it.
    const auto retroContext = harness.contexts.get("1");
    const auto first = retroContext.get("getUV").call({}, {});
    const auto second = retroContext.get("getUV").call({}, {});
    REQUIRE(first.get("isNode").strictEquals(true));
    REQUIRE(first.sameValue(second));
}
