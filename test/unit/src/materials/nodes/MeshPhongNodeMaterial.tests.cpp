#include "materials/nodes/MeshPhongNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("MeshPhongNodeMaterial applies the phong defaults and its own fields") {
    const auto module = nativeMeshPhongNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("MeshPhongNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("MeshPhongNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("MeshPhongNodeMaterial"));
    REQUIRE(material.get("isMeshPhongNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    REQUIRE(material.get("lights").strictEquals(true));
    // The two optional inputs start absent, as the source sets them before the defaults.
    REQUIRE(material.get("shininessNode").isNull());
    REQUIRE(material.get("specularNode").isNull());
    REQUIRE(material.get("shininess").toNumber() == Approx(30));
    REQUIRE(material.get("specular").get("isColor").strictEquals(true));
    for (const char* method : {"setupEnvironment", "setupLightingModel", "setupVariants"}) {
        REQUIRE(material.get(method).get("length").strictEquals(1));
    }
}

TEST_CASE("MeshPhongNodeMaterial answers the plain phong lighting model") {
    const auto material = nativeMeshPhongNodeMaterialConstructor().construct();
    const auto model = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    // Unlike the lambert material this one leaves specular on.
    REQUIRE(model.get("specular").strictEquals(true));
    REQUIRE(model.get("direct").get("length").strictEquals(1));
    REQUIRE(model.get("indirect").get("length").strictEquals(1));
    const auto again = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    REQUIRE_FALSE(again.sameValue(model));
}

TEST_CASE("MeshPhongNodeMaterial setupEnvironment wraps the base result and setupVariants runs") {
    const auto material = nativeMeshPhongNodeMaterialConstructor().construct();
    auto builderMaterial = JSValue::object(nullptr);
    auto builder = JSValue::object(nullptr);
    builder.set("material", builderMaterial);
    // No environment means the wrapper answers null rather than an empty wrapper.
    REQUIRE(material.get("setupEnvironment").call(material, {builder}).isNull());
    // setupVariants assigns the shininess and the specular colour into the shared property nodes; the
    // graph has to build without a render context.
    REQUIRE_NOTHROW(material.get("setupVariants").call(material, {builder}));
}
