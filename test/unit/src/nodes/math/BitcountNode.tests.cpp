#include "nodes/math/BitcountNode.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("BitcountNode exports the pinned constructors and helpers") {
    const auto module = nativeBitcountNodeModule();
    REQUIRE(module.get("default").sameValue(nativeBitcountNodeConstructor()));
    REQUIRE(nativeBitcountNodeConstructor().get("name").strictEquals("BitcountNode"));
    REQUIRE(nativeBitcountNodeConstructor().get("length").strictEquals(2));
    for (const auto* name : {"countTrailingZeros", "countLeadingZeros", "countOneBits"})
        REQUIRE(module.get(name).isCallable());
    REQUIRE(nativeBitcountNodeConstructor().get("COUNT_TRAILING_ZEROS").strictEquals("countTrailingZeros"));
    REQUIRE(nativeBitcountNodeConstructor().get("COUNT_LEADING_ZEROS").strictEquals("countLeadingZeros"));
    REQUIRE(nativeBitcountNodeConstructor().get("COUNT_ONE_BITS").strictEquals("countOneBits"));
}

TEST_CASE("BitcountNode preserves method and input metadata") {
    const auto input = nativeTSLBaseBinding("uint").call({}, {7});
    const auto result = nativeBitcountNodeBinding("countOneBits").call({}, {input});
    const auto node = result.get("isBitcountNode").strictEquals(true) ? result : result.get("node");
    REQUIRE(node.get("type").strictEquals("BitcountNode"));
    REQUIRE(node.get("method").strictEquals("countOneBits"));
    REQUIRE(node.get("aNode").sameValue(input));
    REQUIRE(node.get("isBitcountNode").strictEquals(true));
}
