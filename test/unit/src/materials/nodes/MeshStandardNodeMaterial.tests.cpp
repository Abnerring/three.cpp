#include "materials/nodes/MeshStandardNodeMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/StackNode.h"
#include "nodes/functions/PhysicalLightingModel.h"
#include "nodes/tsl/TSLBase.h"
#include "../../../utils/parity/MeshStandardNodeMaterialFixtures.h"

#include "catch.hpp"

namespace {
namespace F = MeshStandardNodeMaterialFixtures;

JSValue unwrap(JSValue node) {
    while (node.get("isVarNode").strictEquals(true)) node = node.get("node");
    return node;
}
JSValue assignments(const JSValue& stack) {
    auto out = JSValue::object();
    for (unsigned i = 0; i < stack.get("nodes").length(); ++i) {
        const auto node = stack.get("nodes").get(JSValue(i));
        if (node.get("isAssignNode").strictEquals(true)) out.set(node.get("targetNode").get("name").string(), node);
    }
    return out;
}
bool hasEvent(const JSValue& events, const char* label) {
    for (unsigned i = 0; i < events.length(); ++i) if (events.get(JSValue(i)).strictEquals(label)) return true;
    return false;
}
}

TEST_CASE("MeshStandardNodeMaterial preserves class metadata, inheritance and defaults") {
    const auto ctor = nativeMeshStandardNodeMaterialConstructor(), base = nativeNodeMaterialConstructor();
    const auto proto = ctor.get("prototype"), material = ctor.construct(), other = ctor.construct();
    REQUIRE(ctor.get("name").strictEquals("MeshStandardNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("MeshStandardNodeMaterial"));
    REQUIRE(ctor.getPrototype().sameValue(base));
    REQUIRE(proto.getPrototype().sameValue(base.get("prototype")));
    REQUIRE(material.get("type").strictEquals("MeshStandardNodeMaterial"));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isMeshStandardNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isMeshStandardMaterial").strictEquals(true));
    REQUIRE(material.get("lights").strictEquals(true));
    for (const char* name : {"emissiveNode", "metalnessNode", "roughnessNode"}) REQUIRE(material.get(name).isNull());
    REQUIRE(material.get("roughness").strictEquals(1));
    REQUIRE(material.get("metalness").strictEquals(0));
    REQUIRE(material.get("defines").get("STANDARD").strictEquals(""));
    for (const char* name : {"color", "emissive", "normalScale", "envMapRotation"})
        REQUIRE_FALSE(material.get(name).sameValue(other.get(name)));
    for (const auto& entry : std::initializer_list<std::pair<const char*, unsigned>>{
             {"setupEnvironment", 1}, {"setupLightingModel", 0}, {"setupSpecular", 0}, {"setupVariants", 0}}) {
        const auto d = *proto.getOwnPropertyDescriptor(entry.first);
        REQUIRE(d.writable.value_or(false));
        REQUIRE(d.configurable.value_or(false));
        REQUIRE_FALSE(d.enumerable.value_or(false));
        REQUIRE(d.value->get("name").strictEquals(entry.first));
        REQUIRE(d.value->get("length").strictEquals(entry.second));
        REQUIRE_FALSE(d.value->isConstructible());
    }
    const auto getter = *ctor.getOwnPropertyDescriptor("type");
    REQUIRE(getter.configurable.value_or(false));
    REQUIRE_FALSE(getter.enumerable.value_or(false));
    REQUIRE((!getter.setter || getter.setter->isUndefined()));
    REQUIRE(getter.getter->get("name").strictEquals("get type"));
    REQUIRE(getter.getter->get("length").strictEquals(0));
}

TEST_CASE("MeshStandardNodeMaterial parameters and constructor hooks preserve initialization order and reentry") {
    const auto configured = F::constructorCase(F::record({{"parameters", true}}));
    const auto scalars = configured.get("snapshot").get("scalars");
    REQUIRE(scalars.get("roughness").strictEquals(0.23));
    REQUIRE(scalars.get("metalness").strictEquals(0.81));
    REQUIRE(scalars.get("emissiveIntensity").strictEquals(2.5));
    REQUIRE(configured.get("metalnessInput").strictEquals(true));
    REQUIRE(configured.get("emissiveInput").strictEquals(true));
    const auto result = F::constructorHooks(F::record({{"reenter", true}}));
    REQUIRE(result.get("error").isNull());
    REQUIRE(result.get("derived").strictEquals(true));
    const auto events = result.get("events");
    REQUIRE(events.get("0").strictEquals("setDefaultValues:outer"));
    REQUIRE(events.get("1").get("flag").strictEquals(true));
    REQUIRE(events.get("1").get("lights").strictEquals(true));
    REQUIRE(events.get("1").get("emissiveNull").strictEquals(true));
    REQUIRE(events.get("1").get("metalnessNull").strictEquals(true));
    REQUIRE(events.get("1").get("roughnessNull").strictEquals(true));
    REQUIRE(events.get("2").strictEquals(true));
    REQUIRE(events.get("3").strictEquals("setDefaultValues:nested"));
    REQUIRE(events.get("5").strictEquals(true)); // Same module-level defaults in both constructors.
    REQUIRE(events.get("6").strictEquals("setValues:nested"));
    REQUIRE(events.get("8").strictEquals("nested"));
    REQUIRE(events.get("9").strictEquals("setValues:outer"));
    REQUIRE(events.get("11").strictEquals("outer"));
    for (const char* throwAt : {"setDefaultValues:outer", "setValues:outer", "setDefaultValues:nested", "setValues:nested"}) {
        const auto failed = F::constructorHooks(F::record({{"reenter", true}, {"throwAt", throwAt}}));
        REQUIRE(failed.get("error").strictEquals("TypeError"));
        REQUIRE_FALSE(failed.get("derived").toBoolean());
    }
}

TEST_CASE("MeshStandardNodeMaterial EnvironmentNode fallback uses strict null and reevaluates the builder getter") {
    const auto fallback = F::environmentCase(F::record({{"base", nullptr}, {"fallback", "@node"}}));
    REQUIRE(fallback.get("error").isNull());
    REQUIRE(fallback.get("type").strictEquals("EnvironmentNode"));
    REQUIRE(fallback.get("envIsFallback").strictEquals(true));
    REQUIRE(hasEvent(fallback.get("events"), "environment:1"));
    REQUIRE(hasEvent(fallback.get("events"), "environment:2"));
    for (const auto& base : F::Args{false, 0, "", "@undefined"}) {
        const auto result = F::environmentCase(F::record({{"base", base}, {"fallback", "@node"}}));
        REQUIRE(result.get("error").isNull());
        REQUIRE(result.get("isNull").strictEquals(true));
        REQUIRE_FALSE(hasEvent(result.get("events"), "environment:1"));
    }
    const auto changed = F::environmentCase(F::record({{"base", nullptr}, {"fallback", "@node"}, {"second", nullptr}}));
    REQUIRE(changed.get("isNull").strictEquals(true));
    REQUIRE(hasEvent(changed.get("events"), "environment:2"));
    for (const char* throwAt : {"super:get", "super:call", "environment:1", "environment:2"}) {
        const auto result = F::environmentCase(F::record({{"base", nullptr}, {"fallback", "@node"}, {"throwAt", throwAt}}));
        REQUIRE(result.get("error").strictEquals("TypeError"));
    }
    REQUIRE(F::environmentCase(F::record({{"base", nullptr}, {"fallback", "@node"}, {"reenter", true}})).get("error").isNull());
}

TEST_CASE("MeshStandardNodeMaterial preserves the real NodeMaterial environment selection and physical model defaults") {
    const auto material = nativeMeshStandardNodeMaterialConstructor().construct();
    auto builder = JSValue::object();
    REQUIRE(F::call(material, "setupEnvironment", {builder}).isNull());
    const auto sceneEnvironment = nativeTSLBaseBinding("vec3").call({}, {0.6});
    builder.set("environmentNode", sceneEnvironment);
    REQUIRE(F::call(material, "setupEnvironment", {builder}).get("envNode").sameValue(sceneEnvironment));
    const auto ownEnvironment = nativeTSLBaseBinding("vec3").call({}, {0.3});
    auto self = material; self.set("envNode", ownEnvironment);
    REQUIRE(F::call(material, "setupEnvironment", {builder}).get("envNode").sameValue(ownEnvironment));
    self.set("envNode", nullptr); self.set("envMap", F::record({{"isCubeTexture", true}}));
    const auto reference = F::call(material, "setupEnvironment", {builder}).get("envNode");
    REQUIRE(reference.get("property").strictEquals("envMap"));
    REQUIRE(reference.get("uniformType").strictEquals("cubeTexture"));
    REQUIRE_THROWS_AS(F::call(nativeMeshStandardNodeMaterialConstructor().construct(), "setupEnvironment", {nullptr}), JSTypeError);
    const auto model = F::call(material, "setupLightingModel");
    REQUIRE(model.getPrototype().sameValue(nativePhysicalLightingModelConstructor().get("prototype")));
    REQUIRE_FALSE(model.sameValue(F::call(material, "setupLightingModel")));
    for (const char* field : {"clearcoat", "sheen", "iridescence", "anisotropy", "transmission", "dispersion"})
        REQUIRE(model.get(field).strictEquals(false));
}

TEST_CASE("MeshStandardNodeMaterial setupVariants constructs the complete standard TSL assignment graph") {
    const auto material = nativeMeshStandardNodeMaterialConstructor().construct();
    const auto stack = nativeStackNodeConstructor().construct();
    F::RestoreStack restore;
    nativeSetCurrentStack(stack);
    REQUIRE(F::call(material, "setupVariants").isUndefined());
    const auto assigned = assignments(stack);
    REQUIRE(assigned.keys().size() == 6);
    REQUIRE(assigned.get("Metalness").get("sourceNode").sameValue(nativeMaterialNodeBinding("materialMetalness")));
    const auto roughness = unwrap(assigned.get("Roughness").get("sourceNode"));
    REQUIRE(roughness.get("isShaderCallNodeInternal").strictEquals(true));
    REQUIRE(roughness.get("rawInputs").get("0").get("roughness").sameValue(nativeMaterialNodeBinding("materialRoughness")));
    const auto specular = unwrap(assigned.get("SpecularColor").get("sourceNode"));
    REQUIRE(specular.get("type").strictEquals("ConstNode"));
    REQUIRE(specular.get("nodeType").strictEquals("vec3"));
    for (const char* axis : {"x", "y", "z"}) REQUIRE(specular.get("value").get(axis).strictEquals(0.04));
    const auto blend = unwrap(assigned.get("SpecularColorBlended").get("sourceNode"));
    REQUIRE(blend.get("method").strictEquals("mix"));
    REQUIRE(blend.get("bNode").get("node").sameValue(nativePropertyNodeBinding("diffuseColor")));
    REQUIRE(blend.get("bNode").get("components").strictEquals("xyz"));
    REQUIRE(blend.get("cNode").sameValue(nativePropertyNodeBinding("metalness")));
    REQUIRE(unwrap(assigned.get("SpecularF90").get("sourceNode")).get("value").strictEquals(1));
    const auto diffuse = unwrap(assigned.get("DiffuseContribution").get("sourceNode"));
    REQUIRE(diffuse.get("op").strictEquals("*"));
    REQUIRE(diffuse.get("aNode").get("node").sameValue(nativePropertyNodeBinding("diffuseColor")));
    REQUIRE(unwrap(diffuse.get("bNode")).get("method").strictEquals("oneMinus"));
    REQUIRE(unwrap(diffuse.get("bNode")).get("aNode").sameValue(nativeMaterialNodeBinding("materialMetalness")));
    const auto body = roughness.get("shaderNode").get("jsFunc").call({}, {roughness.get("rawInputs").get("0")});
    REQUIRE(unwrap(body).get("method").strictEquals("min"));
    REQUIRE(unwrap(body).get("bNode").get("value").strictEquals(1));
    const auto addition = unwrap(unwrap(body).get("aNode"));
    REQUIRE(addition.get("op").strictEquals("+"));
    const auto floor = unwrap(addition.get("aNode"));
    REQUIRE(floor.get("method").strictEquals("max"));
    REQUIRE(floor.get("aNode").sameValue(nativeMaterialNodeBinding("materialRoughness")));
    REQUIRE(floor.get("bNode").get("value").strictEquals(0.045));
}

TEST_CASE("MeshStandardNodeMaterial custom nodes retain their own metalness input in diffuse and global metalness in specular") {
    auto material = nativeMeshStandardNodeMaterialConstructor().construct();
    const auto input = nativeTSLBaseBinding("float").call({}, {0.65});
    material.set("metalnessNode", input); material.set("roughnessNode", 0.18);
    const auto stack = nativeStackNodeConstructor().construct();
    F::RestoreStack restore; nativeSetCurrentStack(stack);
    F::call(material, "setupVariants");
    const auto assigned = assignments(stack);
    const auto metalnessSource = assigned.get("Metalness").get("sourceNode");
    const auto converted = unwrap(metalnessSource);
    REQUIRE(converted.get("type").strictEquals("ConvertNode"));
    REQUIRE(converted.get("convertTo").strictEquals("float"));
    REQUIRE(converted.get("node").sameValue(input));
    REQUIRE(unwrap(unwrap(assigned.get("DiffuseContribution").get("sourceNode")).get("bNode")).get("aNode").sameValue(metalnessSource));
    REQUIRE(unwrap(assigned.get("SpecularColorBlended").get("sourceNode")).get("cNode").sameValue(nativePropertyNodeBinding("metalness")));
    REQUIRE(unwrap(unwrap(assigned.get("Roughness").get("sourceNode")).get("rawInputs").get("0").get("roughness")).get("value").strictEquals(0.18));
}

TEST_CASE("MeshStandardNodeMaterial assignment and override exceptions preserve exact preceding effects") {
    const auto request = F::record({{"observeAssign", true}, {"metalnessNode", F::array({true, 0.43})},
        {"roughnessNode", F::array({true, 0.67})}, {"throwAt", "call:setupSpecular"}});
    const auto failed = F::variantsCase(request);
    REQUIRE(failed.get("error").strictEquals("TypeError"));
    REQUIRE(hasEvent(failed.get("events"), "get:metalnessNode:2"));
    REQUIRE(hasEvent(failed.get("events"), "get:roughnessNode:2"));
    REQUIRE(hasEvent(failed.get("events"), "call:assign:metalness"));
    REQUIRE(hasEvent(failed.get("events"), "call:assign:roughness"));
    REQUIRE_FALSE(hasEvent(failed.get("events"), "get:assign:specularColor"));
    REQUIRE_FALSE(hasEvent(failed.get("events"), "get:assign:diffuseContribution"));
    for (const char* target : {"metalness", "roughness", "specularColor", "specularColorBlended", "specularF90", "diffuseContribution"}) {
        const auto result = F::variantsCase(F::record({{"observeAssign", true}, {"throwAt", std::string("call:assign:") + target}}));
        REQUIRE(result.get("error").strictEquals("TypeError"));
    }
    REQUIRE(F::variantsCase(F::record({{"observeAssign", true}, {"reenter", true}})).get("error").isNull());
    REQUIRE(F::variantsCase(F::record({{"observeAssign", true}, {"overrideSpecular", true}})).get("error").isNull());
}

TEST_CASE("MeshStandardNodeMaterial setupSpecular does not consult the receiver") {
    const auto proto = nativeMeshStandardNodeMaterialConstructor().get("prototype");
    const auto stack = nativeStackNodeConstructor().construct();
    F::RestoreStack restore; nativeSetCurrentStack(stack);
    REQUIRE(proto.get("setupSpecular").call(nullptr).isUndefined());
    const auto assigned = assignments(stack);
    REQUIRE(assigned.keys().size() == 3);
    REQUIRE(assigned.has("SpecularColor"));
    REQUIRE(assigned.has("SpecularColorBlended"));
    REQUIRE(assigned.has("SpecularF90"));
    REQUIRE_THROWS_AS(proto.get("setupVariants").call(nullptr), JSTypeError);
}
