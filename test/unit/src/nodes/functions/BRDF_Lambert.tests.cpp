#include "nodes/functions/BSDF/BRDF_Lambert.h"

#include "catch.hpp"

using Args = std::vector<JSValue>;

TEST_CASE("BRDF_Lambert exports the pinned Fn") {
    const auto module = nativeBRDFLambertModule();
    const auto function = nativeBRDFLambert();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("BRDF_Lambert multiplies diffuseColor by one over pi") {
    auto diffuse = JSValue::object();
    diffuse.set("mul", JSValue::function(
        +[](const JSValue&, const Args& args, const Args&) -> JSValue { return args.empty() ? JSValue{} : args[0]; },
        {}, "mul", 1));
    auto inputs = JSValue::object();
    inputs.set("diffuseColor", diffuse);

    const auto result = nativeBRDFLambert().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}
