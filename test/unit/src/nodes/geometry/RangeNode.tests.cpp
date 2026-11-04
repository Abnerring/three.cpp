#include "nodes/geometry/RangeNode.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("RangeNode exports the pinned constructor and range helper") {
    const auto module = nativeRangeNodeModule();
    REQUIRE(module.get("default").sameValue(nativeRangeNodeConstructor()));
    REQUIRE(nativeRangeNodeConstructor().get("name").strictEquals("RangeNode"));
    REQUIRE(nativeRangeNodeConstructor().get("length").strictEquals(2));
    REQUIRE(module.get("range").isCallable());
}

TEST_CASE("RangeNode preserves defaults, salt and cacheability") {
    const auto value = nativeRangeNodeBinding("range").call({}, {});
    REQUIRE(value.get("type").strictEquals("RangeNode"));
    REQUIRE(value.get("minNode").get("type").strictEquals("ConstNode"));
    REQUIRE(value.get("maxNode").get("type").strictEquals("ConstNode"));
    REQUIRE(value.get("_salt").strictEquals(0));
    REQUIRE(value.get("isCacheable").call(value).strictEquals(false));
}

TEST_CASE("RangeNode keeps explicit bounds and increments salt") {
    const auto minNode = nativeTSLBaseBinding("float").call({}, {1});
    const auto maxNode = nativeTSLBaseBinding("float").call({}, {2});
    const auto value = nativeRangeNodeBinding("range").call({}, {minNode, maxNode});
    REQUIRE(value.get("type").strictEquals("RangeNode"));
    REQUIRE(value.get("minNode").sameValue(minNode));
    REQUIRE(value.get("maxNode").sameValue(maxNode));
    REQUIRE(value.get("_salt").strictEquals(2246822507.0));
}
