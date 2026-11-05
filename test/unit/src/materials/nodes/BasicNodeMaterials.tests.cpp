#include "materials/nodes/LineBasicNodeMaterial.h"
#include "materials/nodes/MeshNormalNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"

#include "catch.hpp"

TEST_CASE("LineBasicNodeMaterial applies the line basic defaults") {
    const auto model = nativeLineBasicNodeMaterialModule();
    const auto ctor = model.get("default");
    REQUIRE(ctor.get("name").strictEquals("LineBasicNodeMaterial"));
    // constructor( parameters ) has no default, so Function.length is 1.
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("LineBasicNodeMaterial"));

    const auto material = ctor.construct();
    // The instance type resolves through the class accessor.
    REQUIRE(material.get("type").strictEquals("LineBasicNodeMaterial"));
    REQUIRE(material.get("isLineBasicNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));

    // The defaults come from the line basic material, not from the node material base.
    REQUIRE(material.get("fog").strictEquals(true));
    REQUIRE(material.get("linewidth").toNumber() == Approx(1));
    REQUIRE(material.get("linecap").strictEquals("round"));
    REQUIRE(material.get("linejoin").strictEquals("round"));
    REQUIRE(material.get("color").get("isColor").strictEquals(true));
    REQUIRE(material.get("color").get("r").toNumber() == Approx(1));
}

TEST_CASE("LineBasicNodeMaterial accepts parameters") {
    auto parameters = JSValue::object(nullptr);
    parameters.set("fog", false);
    parameters.set("linewidth", 3);
    const auto material = nativeLineBasicNodeMaterialConstructor().construct({parameters});
    REQUIRE(material.get("fog").strictEquals(false));
    REQUIRE(material.get("linewidth").toNumber() == Approx(3));
}

TEST_CASE("MeshNormalNodeMaterial applies the mesh normal defaults") {
    const auto model = nativeMeshNormalNodeMaterialModule();
    const auto ctor = model.get("default");
    REQUIRE(ctor.get("name").strictEquals("MeshNormalNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("MeshNormalNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("MeshNormalNodeMaterial"));
    REQUIRE(material.get("isMeshNormalNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    REQUIRE(material.get("flatShading").strictEquals(false));
    REQUIRE(material.get("wireframe").strictEquals(false));
    // The normal material has no diffuse colour; the setup writes the packed normal instead.
    REQUIRE(material.get("setupDiffuseColor").get("length").strictEquals(0));
}

TEST_CASE("MeshNormalNodeMaterial setupDiffuseColor builds the packed normal graph") {
    const auto material = nativeMeshNormalNodeMaterialConstructor().construct();
    // The setup assigns into the shared diffuse colour property node; without a builder it still has to
    // build the node graph rather than fail.
    REQUIRE_NOTHROW(material.get("setupDiffuseColor").call(material, {}));
}

TEST_CASE("Node materials define their id with a value only descriptor, as the pinned class does") {
    const auto line = nativeLineBasicNodeMaterialConstructor().construct();
    // The pinned Material constructor uses Object.defineProperty with a descriptor that only carries a
    // value, so id is an own property that Object.keys does not list and that cannot be written or
    // reconfigured. The differential compares this descriptor against the pinned module as well.
    const auto descriptor = line.getOwnPropertyDescriptor("id");
    REQUIRE(descriptor.has_value());
    REQUIRE(descriptor->writable.value_or(true) == false);
    REQUIRE(descriptor->enumerable.value_or(true) == false);
    REQUIRE(descriptor->configurable.value_or(true) == false);
    REQUIRE(line.get("id").type() == JSValue::Type::Number);

    // Object.keys hides it while own property names list it, which is exactly the distinction that made
    // an earlier comparison look like a deviation.
    const auto names = line.ownPropertyNames();
    bool hasId = false;
    for (const auto& name : names) { if (name == "id") hasId = true; }
    REQUIRE(hasId);
}