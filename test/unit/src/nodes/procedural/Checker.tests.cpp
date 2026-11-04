#include "nodes/procedural/Checker.h"

#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("Checker exports the pinned procedural Fn") {
    const auto module = nativeCheckerModule();
    const auto value = nativeChecker();
    REQUIRE(module.get("checker").sameValue(value));
    REQUIRE(value.get("type").strictEquals("Node"));
    REQUIRE(value.get("isFn").strictEquals(true));
    REQUIRE(value.get("isNode").strictEquals(true));
    REQUIRE(value.get("length").isCallable());
}

TEST_CASE("Checker accepts the default UV and explicit coordinates") {
    const auto checker = nativeChecker();
    const auto coord = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
    const auto value = checker.call({}, {coord});
    REQUIRE(value.get("isNode").strictEquals(true));
    REQUIRE(value.get("node").get("rawInputs").get("length").strictEquals(1));
    const auto defaultValue = checker.call();
    REQUIRE(defaultValue.get("isNode").strictEquals(true));
}
