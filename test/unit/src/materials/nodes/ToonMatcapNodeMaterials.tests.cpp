#include "materials/nodes/MeshMatcapNodeMaterial.h"
#include "materials/nodes/MeshToonNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("MeshToonNodeMaterial applies the toon defaults and its own flags") {
    const auto module = nativeMeshToonNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("MeshToonNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("MeshToonNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("MeshToonNodeMaterial"));
    REQUIRE(material.get("isMeshToonNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    // The toon material is lit, and the defaults come from the toon material.
    REQUIRE(material.get("lights").strictEquals(true));
    REQUIRE(material.get("fog").strictEquals(true));
    REQUIRE(material.get("gradientMap").isNull());
    REQUIRE(material.get("setupLightingModel").get("length").strictEquals(1));
}

TEST_CASE("MeshToonNodeMaterial setupLightingModel answers a toon lighting model") {
    const auto material = nativeMeshToonNodeMaterialConstructor().construct();
    const auto model = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    // The toon model implements direct with the builder and indirect with the builder alone.
    REQUIRE(model.get("direct").get("length").strictEquals(2));
    REQUIRE(model.get("indirect").get("length").strictEquals(1));
    REQUIRE(model.get("start").isCallable());
    // Each call answers a new model, as the source constructs one every time.
    const auto again = material.get("setupLightingModel").call(material, {JSValue::object(nullptr)});
    REQUIRE_FALSE(again.sameValue(model));
}

TEST_CASE("MeshMatcapNodeMaterial applies the matcap defaults and its own flags") {
    const auto module = nativeMeshMatcapNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("MeshMatcapNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("MeshMatcapNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("MeshMatcapNodeMaterial"));
    REQUIRE(material.get("isMeshMatcapNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    REQUIRE(material.get("matcap").isNull());
    REQUIRE(material.get("setupVariants").get("length").strictEquals(1));
}

TEST_CASE("MeshMatcapNodeMaterial setupVariants builds the default matcap colour") {
    const auto material = nativeMeshMatcapNodeMaterialConstructor().construct();
    auto builderMaterial = JSValue::object(nullptr);
    builderMaterial.set("matcap", nullptr);
    auto builder = JSValue::object(nullptr);
    builder.set("material", builderMaterial);
    // Without a matcap the source mixes a vertical gradient and multiplies it into the diffuse colour;
    // the graph has to build without a render context.
    REQUIRE_NOTHROW(material.get("setupVariants").call(material, {builder}));
}
