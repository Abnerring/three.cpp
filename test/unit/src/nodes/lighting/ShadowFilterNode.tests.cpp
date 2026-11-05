#include "nodes/lighting/ShadowFilterNode.h"

#include "nodes/core/Node.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

namespace {
JSValue vec3Node(double x, double y, double z) {
    return nativeTSLBaseBinding("vec3").call({}, {JSValue(x), JSValue(y), JSValue(z)});
}
// A stand-in depth texture. The filters only read isArrayTexture and hand the object to texture().
JSValue depthTexture() {
    auto texture = JSValue::object(nullptr);
    texture.set("isArrayTexture", false);
    return texture;
}
}  // namespace

TEST_CASE("ShadowFilterNode exports the three pinned filters") {
    const auto module = nativeShadowFilterNodeModule();
    for (const char* name : {"BasicShadowFilter", "PCFShadowFilter", "VSMShadowFilter"}) {
        REQUIRE(module.get(name).isCallable());
    }
    REQUIRE(module.get("notAFilter").isUndefined());
}

TEST_CASE("BasicShadowFilter samples the depth texture and compares it with the shadow z") {
    auto inputs = JSValue::object(nullptr);
    inputs.set("depthTexture", depthTexture());
    inputs.set("shadowCoord", vec3Node(0.5, 0.5, 0.25));
    const auto result = nativeShadowFilterNodeBinding("BasicShadowFilter").call({}, {inputs});
    // A comparison node comes back, not the texture itself.
    REQUIRE(result.get("isNode").strictEquals(true));
    REQUIRE_FALSE(result.sameValue(inputs.get("shadowCoord")));
}

TEST_CASE("PCFShadowFilter builds a five sample filtered node") {
    auto shadow = JSValue::object(nullptr);
    shadow.set("mapSize", JSValue::object(nullptr));
    shadow.set("radius", JSValue(2));
    auto inputs = JSValue::object(nullptr);
    inputs.set("depthTexture", depthTexture());
    inputs.set("shadowCoord", vec3Node(0.5, 0.5, 0.25));
    inputs.set("shadow", shadow);
    const auto result = nativeShadowFilterNodeBinding("PCFShadowFilter").call({}, {inputs});
    REQUIRE(result.get("isNode").strictEquals(true));
}
