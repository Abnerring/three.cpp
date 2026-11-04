#include "nodes/math/BitcastNode.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("BitcastNode exports the pinned constructors and helpers") {
    const auto module = nativeBitcastNodeModule();
    REQUIRE(module.get("default").sameValue(nativeBitcastNodeConstructor()));
    REQUIRE(nativeBitcastNodeConstructor().get("name").strictEquals("BitcastNode"));
    REQUIRE(nativeBitcastNodeConstructor().get("length").strictEquals(2));
    for (const auto* name : {"bitcast", "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat"})
        REQUIRE(module.get(name).isCallable());
}

TEST_CASE("BitcastNode direct helpers preserve conversion metadata") {
    const auto value = nativeTSLBaseBinding("float").call({}, {1.5});
    const auto result = nativeBitcastNodeBinding("floatBitsToInt").call({}, {value});
    REQUIRE(result.get("type").strictEquals("BitcastNode"));
    REQUIRE(result.get("valueNode").sameValue(value));
    REQUIRE(result.get("conversionType").strictEquals("int"));
    REQUIRE(result.get("inputType").strictEquals("float"));
    REQUIRE(result.get("isBitcastNode").strictEquals(true));
}

TEST_CASE("BitcastNode intent helper wraps the node as a variable") {
    const auto value = nativeTSLBaseBinding("float").call({}, {1.5});
    const auto result = nativeBitcastNodeBinding("bitcast").call({}, {value, "uint"});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("BitcastNode"));
    REQUIRE(result.get("node").get("conversionType").strictEquals("uint"));
    REQUIRE(result.get("node").get("inputType").isNull());
}
