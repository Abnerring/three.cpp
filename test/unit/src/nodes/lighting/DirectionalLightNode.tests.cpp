#include "nodes/lighting/DirectionalLightNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("DirectionalLightNode exposes the pinned directional node shape") {
    const auto module = nativeDirectionalLightNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("DirectionalLightNode"));
    // constructor( light = null ) has a default, so Function.length is 0.
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("DirectionalLightNode"));
    const auto node = ctor.construct();
    REQUIRE(node.get("type").strictEquals("DirectionalLightNode"));
    REQUIRE(node.get("light").isNull());
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    REQUIRE(node.get("colorNode").get("isNode").strictEquals(true));
    // setupDirect() takes no parameters in this subclass.
    REQUIRE(node.get("setupDirect").isCallable());
    REQUIRE(node.get("setupDirect").get("length").strictEquals(0));
}

TEST_CASE("DirectionalLightNode setupDirect returns the colour and a target direction") {
    // The light accessors accept any object value, as the Lights accessor tests establish, so a
    // plain object is enough to exercise the real helper rather than a stub.
    auto light = JSValue::object(nullptr);
    const auto node = nativeDirectionalLightNodeConstructor().construct({light});
    REQUIRE(node.get("light").sameValue(light));

    const auto result = node.get("setupDirect").call(node, {});

    // The object literal is { lightDirection, lightColor }.
    REQUIRE(result.get("lightColor").sameValue(node.get("colorNode")));
    REQUIRE(result.get("lightDirection").get("isNode").strictEquals(true));
    REQUIRE_FALSE(result.get("lightDirection").get("type").strictEquals("UniformNode"));

    // The direction node is a fresh one per call, not a cached shared node.
    const auto again = node.get("setupDirect").call(node, {});
    REQUIRE_FALSE(again.get("lightDirection").sameValue(result.get("lightDirection")));
}
