#include "nodes/functions/BSDF/BRDF_EON.h"
#include "nodes/tsl/TSLBase.h"
#include "../../../utils/parity/BRDFEONScenarios.h"

#include "catch.hpp"

TEST_CASE("BRDF_EON exports the pinned Fn") {
    const auto module = nativeBRDFEONModule();
    const auto function = nativeBRDFEON();
    REQUIRE(module.get("default").sameValue(function));
    REQUIRE(function.get("isFn").toBoolean());
}

TEST_CASE("BRDF_EON builds a vec3 result") {
    auto inputs = JSValue::object();
    inputs.set("lightDirection", nativeTSLBaseBinding("vec3").call({}, {0.2, 0.7, 0.6}));
    inputs.set("diffuseColor", nativeTSLBaseBinding("vec3").call({}, {0.5, 0.4, 0.3}));
    inputs.set("roughness", nativeTSLBaseBinding("float").call({}, {0.35}));
    inputs.set("normalView", nativeTSLBaseBinding("vec3").call({}, {0.0, 0.0, 1.0}));
    inputs.set("viewDirection", nativeTSLBaseBinding("vec3").call({}, {0.1, 0.2, 0.97}));
    const auto result = nativeBRDFEON().call({}, {inputs});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
}

TEST_CASE("EON directional albedo exports its typed Fn and complete callback") {
    const auto function = nativeEONDirectionalAlbedo();
    REQUIRE(nativeBRDFEONModule().get("EON_DirectionalAlbedo").sameValue(function));
    REQUIRE(nativeBRDFEONBinding("EON_DirectionalAlbedo").sameValue(function));
    REQUIRE(function.get("isFn").strictEquals(true));
    const auto layout = function.get("shaderNode").get("layout");
    REQUIRE(layout.get("type").strictEquals("vec3"));
    REQUIRE(layout.get("inputs").length() == 3);
    REQUIRE(layout.get("inputs").get(JSValue(0)).get("name").strictEquals("diffuseColor"));
    REQUIRE(layout.get("inputs").get(JSValue(0)).get("type").strictEquals("vec3"));
    const auto result = function.call({}, {BRDFEONScenarios::directionalInputs()});
    REQUIRE(result.get("type").strictEquals("VarNode"));
    REQUIRE(result.get("node").get("type").strictEquals("Node"));
    const auto graph = BRDFEONScenarios::Graph()(BRDFEONScenarios::callback(function).call({}, {BRDFEONScenarios::directionalInputs()}));
    const auto serialized = graph.toJSON()->dump();
    for (const auto* term : {"reciprocal", "clamp", "ConditionalNode", "<=", "muComp"})
        REQUIRE(serialized.find(term) != std::string::npos);
}

TEST_CASE("EON FON directional albedo uses the pinned quartic rather than an extra mu complement factor") {
    const auto values = BRDFEONScenarios::fonValues();
    REQUIRE(values.length() == 5);
    REQUIRE(values.get(JSValue(0)).toNumber() == Approx(.831154636886).epsilon(1e-12));
    REQUIRE(values.get(JSValue(2)).toNumber() == Approx(.7520825933610938).epsilon(1e-12));
    REQUIRE(values.get(JSValue(4)).toNumber() == Approx(.7).epsilon(1e-12));
}
