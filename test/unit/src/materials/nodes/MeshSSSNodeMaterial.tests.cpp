#include "../../../utils/parity/MeshSSSNodeMaterialScenarios.h"
#include "catch.hpp"

TEST_CASE("MeshSSSNodeMaterial preserves physical parameters thickness defaults and inherited copy") {
    const auto result = MeshSSSNodeMaterialScenarios::constructorAndCopy();
    REQUIRE(result.get("roughness").strictEquals(0.25));
    REQUIRE(result.get("thickness").strictEquals(2));
    REQUIRE(result.get("initialSSS").strictEquals(false));
    REQUIRE(result.get("enabled").toJSON()->dump() == "[false,true,true,true,true]");
    for (const auto* flag : {"clearcoat", "sheen", "iridescence", "anisotropy", "transmission", "dispersion", "useSSS"})
        REQUIRE(result.get("flags").get(flag).strictEquals(true));
    REQUIRE(result.get("flags").get("retroreflection").strictEquals(false));
    REQUIRE(result.get("flags").get("diffuseRoughness").strictEquals(false));
    REQUIRE(result.get("cloneType").strictEquals("MeshSSSNodeMaterial"));
    for (const auto* key : {"distinctModel", "cloneColorSame", "clonePowerSame", "copyReturnsSelf", "copyColorSame", "customScaleRetained"})
        REQUIRE(result.get(key).strictEquals(true));
    REQUIRE(result.get("copyState").get("receiver").strictEquals(true));
    REQUIRE(result.get("copyState").get("source").strictEquals(true));
}

TEST_CASE("SSS direct builds the full scattering expression before forwarding the reconstructed input") {
    using namespace MeshSSSNodeMaterialScenarios;
    const auto result = directCase(true);
    REQUIRE(result.get("result").strictEquals("undefined"));
    REQUIRE(result.get("graphs").length() == 1);
    auto expression = result.get("graphs").get(JSValue(0));
    while (expression.get("type").strictEquals("VarNode")) expression = expression.get("node");
    REQUIRE(expression.get("type").strictEquals("OperatorNode"));
    REQUIRE(expression.get("op").strictEquals("*"));
    const auto json = expression.toJSON()->dump();
    for (const auto* term : {"@normalView", "@positionViewDirection", "normalize", "dot", "clamp", "pow"})
        REQUIRE(json.find(term) != std::string::npos);
    REQUIRE(result.get("events").get(JSValue(result.get("events").length() - 2)).strictEquals("addAssign"));
    REQUIRE(result.get("events").get(JSValue(result.get("events").length() - 1)).strictEquals("super.direct"));
    const auto forwarded = result.get("forwards").get(JSValue(0));
    REQUIRE(forwarded.get("keys").toJSON()->dump() == "[\"lightDirection\",\"lightColor\",\"reflectedLight\"]");
    for (const auto* key : {"receiver", "builder", "distinctInput", "direction", "color", "reflected"})
        REQUIRE(forwarded.get(key).strictEquals(true));
}

TEST_CASE("SSS direct distinguishes strict enablement failures and reentrant accumulation") {
    using namespace MeshSSSNodeMaterialScenarios;
    for (const auto& gate : Args{false, 1}) {
        const auto result = directCase(gate);
        REQUIRE(result.get("graphs").length() == 0);
        REQUIRE(result.get("forwards").length() == 1);
        REQUIRE(result.get("events").toJSON()->dump() ==
            "[\"input.lightDirection\",\"input.lightColor\",\"input.reflectedLight\",\"self.useSSS\",\"super.direct\"]");
    }
    const auto failed = directCase(true, true);
    REQUIRE(failed.get("result").strictEquals("TypeError"));
    REQUIRE(failed.get("graphs").length() == 1);
    REQUIRE(failed.get("forwards").length() == 0);
    const auto nested = directCase(true, false, true);
    REQUIRE(nested.get("result").strictEquals("undefined"));
    REQUIRE(nested.get("graphs").length() == 2);
    REQUIRE(nested.get("forwards").length() == 2);
}
