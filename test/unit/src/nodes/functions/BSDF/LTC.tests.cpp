#include "nodes/functions/BSDF/LTC.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

namespace {
JSValue vec3Node(double x, double y, double z) {
    return nativeTSLBaseBinding("vec3").call({}, {JSValue(x), JSValue(y), JSValue(z)});
}
JSValue floatNode(double value) { return nativeTSLBaseBinding("float").call({}, {JSValue(value)}); }
}  // namespace

TEST_CASE("LTC exports the three pinned functions") {
    const auto module = nativeLTCModule();
    for (const char* name : {"LTC_Uv", "LTC_Evaluate", "LTC_Evaluate_Volume"}) {
        REQUIRE(module.get(name).isCallable());
    }
    REQUIRE(module.get("notAFunction").isUndefined());
}

TEST_CASE("LTC_Uv builds the lookup coordinate from the roughness and the view dot normal") {
    auto inputs = JSValue::object(nullptr);
    inputs.set("N", vec3Node(0, 0, 1));
    inputs.set("V", vec3Node(0, 0, 1));
    inputs.set("roughness", floatNode(0.5));
    const auto uv = nativeLTCBinding("LTC_Uv").call({}, {inputs});
    REQUIRE(uv.get("isNode").strictEquals(true));
    // The source assigns the scaled coordinate back into the uv before returning it, and that answers a
    // var typed node. The differential runs the same call against the pinned module and compares this
    // type, so the expectation here is not a guess.
    REQUIRE(uv.get("type").strictEquals("VarNode"));
}
