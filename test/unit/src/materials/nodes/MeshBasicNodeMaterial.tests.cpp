#include "materials/nodes/MeshBasicNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/Node.h"
#include "nodes/core/PropertyNode.h"

#include "catch.hpp"

TEST_CASE("MeshBasicNodeMaterial applies the basic defaults and overrides five methods") {
    const auto module = nativeMeshBasicNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("MeshBasicNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("MeshBasicNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("MeshBasicNodeMaterial"));
    REQUIRE(material.get("isMeshBasicNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    REQUIRE(material.get("lights").strictEquals(true));
    REQUIRE(material.get("fog").strictEquals(true));
    REQUIRE(material.get("setupNormal").get("length").strictEquals(0));
    REQUIRE(material.get("setupEnvironment").get("length").strictEquals(1));
    REQUIRE(material.get("setupLightMap").get("length").strictEquals(1));
    REQUIRE(material.get("setupOutgoingLight").get("length").strictEquals(0));
    REQUIRE(material.get("setupLightingModel").get("length").strictEquals(0));
}

TEST_CASE("MeshBasicNodeMaterial setupNormal negates the geometric normal on the back side") {
    const auto material = nativeMeshBasicNodeMaterialConstructor().construct();
    const auto normal = material.get("setupNormal").call(material, {});
    REQUIRE(normal.get("isNode").strictEquals(true));
    // It is the wrapper around the accessor, not the accessor itself.
    REQUIRE_FALSE(normal.sameValue(nativeNormalBinding("normalViewGeometry")));
}

TEST_CASE("MeshBasicNodeMaterial setupOutgoingLight is the diffuse colour") {
    const auto material = nativeMeshBasicNodeMaterialConstructor().construct();
    const auto outgoing = material.get("setupOutgoingLight").call(material, {});
    REQUIRE(outgoing.get("isNode").strictEquals(true));
    REQUIRE(outgoing.sameValue(nativePropertyNodeBinding("diffuseColor").get("rgb")));
}

TEST_CASE("MeshBasicNodeMaterial setupLightMap answers a wrapper only when a light map exists") {
    const auto material = nativeMeshBasicNodeMaterialConstructor().construct();

    auto plainMaterial = JSValue::object(nullptr);
    plainMaterial.set("lightMap", nullptr);
    auto plainBuilder = JSValue::object(nullptr);
    plainBuilder.set("material", plainMaterial);
    REQUIRE(material.get("setupLightMap").call(material, {plainBuilder}).isNull());

    auto mappedMaterial = JSValue::object(nullptr);
    mappedMaterial.set("lightMap", true);
    auto mappedBuilder = JSValue::object(nullptr);
    mappedBuilder.set("material", mappedMaterial);
    const auto wrapped = material.get("setupLightMap").call(material, {mappedBuilder});
    REQUIRE_FALSE(wrapped.isNull());
    REQUIRE(wrapped.get("isNode").strictEquals(true));
}

TEST_CASE("MeshBasicNodeMaterial setupEnvironment and setupLightingModel answer their base pieces") {
    const auto material = nativeMeshBasicNodeMaterialConstructor().construct();
    auto builderMaterial = JSValue::object(nullptr);
    auto builder = JSValue::object(nullptr);
    builder.set("material", builderMaterial);
    // No environment means the wrapper answers null, as in the lambert and phong materials.
    REQUIRE(material.get("setupEnvironment").call(material, {builder}).isNull());

    const auto model = material.get("setupLightingModel").call(material, {});
    // The basic lighting model implements indirect and leaves the other hooks abstract.
    REQUIRE(model.get("indirect").get("length").strictEquals(1));
    REQUIRE(model.get("start").isCallable());
    const auto again = material.get("setupLightingModel").call(material, {});
    REQUIRE_FALSE(again.sameValue(model));
}
