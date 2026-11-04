#include "nodes/functions/material/getRoughness.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("getRoughness exports the pinned Fn") {
    const auto module = nativeGetRoughnessModule();
    const auto function = nativeGetRoughness();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("getRoughness builds a clamped roughness result") {
    auto inputs = JSValue::object();
    inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {0.5}));
    const auto result = nativeGetRoughness().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
