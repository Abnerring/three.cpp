#include "nodes/functions/BSDF/DFGLUT.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("DFGLUT exports the pinned Fn") {
    const auto module = nativeDFGLUTModule();
    const auto function = nativeDFGLUT();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("DFGLUT samples a vec2 lookup result") {
    auto inputs = JSValue::object();
    inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {0.35}));
    inputs.set("dotNV", nativeTSLBaseBinding("float").call({}, {0.8}));
    const auto result = nativeDFGLUT().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
