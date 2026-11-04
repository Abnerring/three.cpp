#include "nodes/functions/material/getGeometryRoughness.h"

#include "catch.hpp"

TEST_CASE("getGeometryRoughness exports the pinned Fn") {
    const auto module = nativeGetGeometryRoughnessModule();
    REQUIRE(module.get("default").sameValue(nativeGetGeometryRoughness()));
    REQUIRE(nativeGetGeometryRoughness().get("isFn").toBoolean());
}

TEST_CASE("getGeometryRoughness returns zero without a normal attribute") {
    auto geometry = JSValue::object();
    geometry.set("hasAttribute", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return false; },
        {}, "hasAttribute", 1));
    auto builder = JSValue::object();
    builder.set("geometry", geometry);
    const auto result = nativeGetGeometryRoughness().call({}, {builder});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
