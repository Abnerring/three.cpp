#include "materials/nodes/MeshLambertNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("MeshLambertNodeMaterial applies the lambert defaults and its own flags") {
    const auto module = nativeMeshLambertNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("MeshLambertNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("MeshLambertNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("MeshLambertNodeMaterial"));
    REQUIRE(material.get("isMeshLambertNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    REQUIRE(material.get("lights").strictEquals(true));
    // Two methods are overridden on top of the node material base.
    REQUIRE(material.get("setupEnvironment").get("length").strictEquals(1));
    REQUIRE(material.get("setupLightingModel").get("length").strictEquals(1));
}

TEST_CASE("MeshLambertNodeMaterial forces the lambert variant of the phong lighting model") {
    const auto material = nativeMeshLambertNodeMaterialConstructor().construct();
    const auto model = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    // The source passes false for specular: "force lambert".
    REQUIRE(model.get("specular").strictEquals(false));
    // The model still carries the phong hooks, so the specular term exists but is skipped at run time.
    REQUIRE(model.get("direct").get("length").strictEquals(1));
    REQUIRE(model.get("indirect").get("length").strictEquals(1));
    REQUIRE(model.get("start").isCallable());
    // Every call answers a new model.
    const auto again = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    REQUIRE_FALSE(again.sameValue(model));
}

TEST_CASE("MeshLambertNodeMaterial setupEnvironment wraps the base result") {
    const auto material = nativeMeshLambertNodeMaterialConstructor().construct();
    auto builderMaterial = JSValue::object(nullptr);
    auto builder = JSValue::object(nullptr);
    builder.set("material", builderMaterial);
    // With no environment the base answers nothing and the wrapper must answer null rather than a
    // wrapper around nothing.
    const auto result = material.get("setupEnvironment").call(material, {builder});
    REQUIRE(result.isNull());
}
