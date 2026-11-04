#include "nodes/functions/BSDF/Schlick_to_F0.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("Schlick_to_F0 exports the pinned typed Fn") {
    const auto module = nativeSchlickToF0Module();
    const auto function = nativeSchlickToF0();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
    REQUIRE(function.get("shaderNode").get("layout").get("type").strictEquals("vec3"));
}

TEST_CASE("Schlick_to_F0 builds the same vector result shape") {
    const auto f = nativeTSLBaseBinding("vec3").call({}, {0.2, 0.3, 0.4});
    const auto f90 = nativeTSLBaseBinding("float").call({}, {0.95});
    const auto dotVH = nativeTSLBaseBinding("float").call({}, {0.5});
    auto inputs = JSValue::object();
    inputs.set("f", f);
    inputs.set("f90", f90);
    inputs.set("dotVH", dotVH);
    const auto result = nativeSchlickToF0().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
