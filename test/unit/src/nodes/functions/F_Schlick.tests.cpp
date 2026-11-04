#include "nodes/functions/BSDF/F_Schlick.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

TEST_CASE("F_Schlick exports the pinned Fn") {
    const auto module = nativeFSchlickModule();
    const auto function = nativeFSchlick();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("F_Schlick builds a vec3 result") {
    auto inputs = JSValue::object();
    inputs.set("f0", nativeTSLBaseBinding("vec3").call({}, {0.2, 0.3, 0.4}));
    inputs.set("f90", nativeTSLBaseBinding("vec3").call({}, {1.0, 1.0, 1.0}));
    inputs.set("dotVH", nativeTSLBaseBinding("float").call({}, {0.5}));
    const auto result = nativeFSchlick().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
