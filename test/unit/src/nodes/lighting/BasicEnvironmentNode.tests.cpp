#include "nodes/lighting/BasicEnvironmentNode.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("BasicEnvironmentNode exposes the pinned lighting node shape") {
    const auto module = nativeBasicEnvironmentNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("BasicEnvironmentNode"));
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("BasicEnvironmentNode"));
    const auto node = ctor.construct();
    REQUIRE(node.get("type").strictEquals("BasicEnvironmentNode"));
    REQUIRE(node.get("isLightingNode").strictEquals(true));
    REQUIRE(node.get("envNode").isNull());
}

TEST_CASE("BasicEnvironmentNode setup writes a contextual environment node") {
    const auto env = nativeNodeConstructor().construct();
    const auto node = nativeBasicEnvironmentNodeConstructor().construct({env});
    auto context = JSValue::object(nullptr);
    auto builder = JSValue::object(nullptr);
    builder.set("context", context);
    node.get("setup").call(node, {builder});
    const auto result = context.get("environment");
    REQUIRE(result.get("isNode").strictEquals(true));
    REQUIRE(result.get("type").strictEquals("ContextNode"));
    REQUIRE(result.get("value").get("forceUVContext").strictEquals(true));
    REQUIRE(result.get("value").get("getUV").isCallable());
}
