#include "nodes/utils/Oscillators.h"

#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("Oscillators exports all pinned wave functions") {
    const auto module = nativeOscillatorsModule();
    for (const auto* name : {"oscSine", "oscSquare", "oscTriangle", "oscSawtooth"})
        REQUIRE(module.get(name).isCallable());
}

TEST_CASE("Oscillators preserve node output types and default timer input") {
    const auto value = nativeTSLBaseBinding("float").call({}, {.25});
    for (const auto* name : {"oscSine", "oscSquare", "oscTriangle", "oscSawtooth"}) {
        const auto node = nativeOscillatorsBinding(name).call({}, {value});
        REQUIRE(node.get("isNode").strictEquals(true));
        REQUIRE(node.get("type").strictEquals("VarNode"));
    }
    const auto sine = nativeOscillatorsBinding("oscSine").call();
    REQUIRE(sine.get("isNode").strictEquals(true));
}
