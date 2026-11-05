#include "materials/nodes/MeshPhysicalNodeMaterial.h"

#include "materials/MeshPhysicalMaterial.h"
#include "materials/nodes/MeshPhysicalNodeMaterialSources.inc"
#include "materials/nodes/MeshStandardNodeMaterial.h"
#include "nodes/accessors/AccessorsUtils.h"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/SubBuildNode.h"
#include "nodes/functions/PhysicalLightingModel.h"
#include "nodes/functions/material/getRoughness.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSValue property(const char* name) { return nativePropertyNodeBinding(name); }
JSValue math(const char* name, const Args& args) { return nativeMathNodeBinding(name).call({}, args); }
JSValue node(const char* type, const Args& args) { return nativeTSLBaseBinding(type).call({}, args); }

// Ternaries intentionally read an overridden property again after its truthiness
// check, just as the source does. Accessors can observe or change that second read.
JSValue overrideNode(const JSValue& self, const char* name, const char* type, const char* fallback) {
    if (self.get(name).toBoolean()) return node(type, {self.get(name)});
    return nativeMaterialNodeBinding(fallback);
}
JSValue roughnessFor(const JSValue& value) {
    auto input = JSValue::object();
    own(input, "roughness", value);
    return nativeGetRoughness().call({}, {input});
}
JSValue superCall(const JSValue& self, const Args& captures, const char* key, const Args& args) {
    return captures.at(0).getPrototype().get(key, self).call(self, args);
}

JSValue useFeature(const JSValue& self, const Args&, const Args& captures) {
    if (greater(self.get(captures.at(0).toString()), JSValue(0.0))) return true;
    return !self.get(captures.at(1).toString()).isNull();
}

JSValue setupSpecular(const JSValue& self, const Args&, const Args&) {
    const auto iorNode = overrideNode(self, "iorNode", "float", "materialIOR");
    const auto specularColorNode = overrideNode(self, "specularColorNode", "vec3", "materialSpecularColor");
    const auto specularIntensityNode = overrideNode(self, "specularIntensityNode", "float", "materialSpecularIntensity");
    invoke(property("ior"), "assign")({iorNode});

    const auto assignColor = invoke(property("specularColor"), "assign");
    const auto numerator = invoke(property("ior"), "sub")({1.0});
    const auto quotient = invoke(numerator, "div")({invoke(property("ior"), "add")({1.0})});
    const auto reflectance = math("pow2", {quotient});
    const auto tinted = invoke(reflectance, "mul")({specularColorNode});
    const auto clamped = math("min", {tinted, node("vec3", {1.0})});
    assignColor({invoke(clamped, "mul")({specularIntensityNode})});
    const auto assignBlended = invoke(property("specularColorBlended"), "assign");
    assignBlended({math("mix", {property("specularColor"), property("diffuseColor").get("rgb"), property("metalness")})});
    const auto assignF90 = invoke(property("specularF90"), "assign");
    assignF90({math("mix", {specularIntensityNode, 1.0, property("metalness")})});
    return {};
}

JSValue setupLightingModel(const JSValue& self, const Args&, const Args&) {
    return nativePhysicalLightingModelConstructor().construct({
        self.get("useClearcoat"), self.get("useSheen"), self.get("useIridescence"),
        self.get("useAnisotropy"), self.get("useTransmission"), self.get("useDispersion"),
        self.get("useRetroreflection"), self.get("useDiffuseRoughness")});
}

JSValue setupVariants(const JSValue& self, const Args& args, const Args& captures) {
    superCall(self, captures, "setupVariants", {arg(args, 0)});

    if (self.get("useDiffuseRoughness").toBoolean()) {
        const auto value = overrideNode(self, "diffuseRoughnessNode", "float", "materialDiffuseRoughness");
        invoke(property("diffuseRoughness"), "assign")({invoke(value, "clamp")()});
    }
    if (self.get("useClearcoat").toBoolean()) {
        const auto value = overrideNode(self, "clearcoatNode", "float", "materialClearcoat");
        const auto roughness = overrideNode(self, "clearcoatRoughnessNode", "float", "materialClearcoatRoughness");
        invoke(property("clearcoat"), "assign")({value});
        invoke(property("clearcoatRoughness"), "assign")({roughnessFor(roughness)});
    }
    if (self.get("useSheen").toBoolean()) {
        const auto value = overrideNode(self, "sheenNode", "vec3", "materialSheen");
        const auto roughness = overrideNode(self, "sheenRoughnessNode", "float", "materialSheenRoughness");
        invoke(property("sheen"), "assign")({value});
        invoke(property("sheenRoughness"), "assign")({roughness});
    }
    if (self.get("useRetroreflection").toBoolean()) {
        const auto value = overrideNode(self, "retroreflectivityNode", "float", "materialRetroreflectivity");
        invoke(property("retroreflectivity"), "assign")({value});
    }
    if (self.get("useIridescence").toBoolean()) {
        const auto value = overrideNode(self, "iridescenceNode", "float", "materialIridescence");
        const auto ior = overrideNode(self, "iridescenceIORNode", "float", "materialIridescenceIOR");
        const auto thickness = overrideNode(self, "iridescenceThicknessNode", "float", "materialIridescenceThickness");
        invoke(property("iridescence"), "assign")({value});
        invoke(property("iridescenceIOR"), "assign")({ior});
        invoke(property("iridescenceThickness"), "assign")({thickness});
    }
    if (self.get("useAnisotropy").toBoolean()) {
        const auto vector = invoke(overrideNode(self, "anisotropyNode", "vec2", "materialAnisotropy"), "toVar")();
        invoke(property("anisotropy"), "assign")({invoke(vector, "length")()});
        const auto condition = invoke(property("anisotropy"), "equal")({0.0});
        auto thenBody = JSValue::function(
            +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                invoke(c.at(0), "assign")({node("vec2", {1.0, 0.0})});
                return {};
            }, {vector}, "", 0);
        auto elseBody = JSValue::function(
            +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                invoke(c.at(0), "divAssign")({node("vec2", {property("anisotropy")})});
                invoke(property("anisotropy"), "assign")({invoke(property("anisotropy"), "saturate")()});
                return {};
            }, {vector}, "", 0);
        thenBody.setFunctionSourceText(MeshPhysicalNodeMaterialSource::anisotropyThenSource);
        elseBody.setFunctionSourceText(MeshPhysicalNodeMaterialSource::anisotropyElseSource);
        const auto branch = nativeTSLBaseBinding("If").call({}, {condition, thenBody});
        invoke(branch, "Else")({elseBody});
        invoke(property("alphaT"), "assign")({invoke(invoke(property("anisotropy"), "pow2")(), "mix")({
            invoke(property("roughness"), "pow2")(), 1.0})});
        const auto matrix = nativeAccessorsUtilsBinding("TBNViewMatrix");
        const auto assignT = invoke(property("anisotropyT"), "assign");
        const auto tangentX = invoke(matrix.get(JSValue(0.0)), "mul")({vector.get("x")});
        assignT({invoke(tangentX, "add")({invoke(matrix.get(JSValue(1.0)), "mul")({vector.get("y")})})});
        const auto assignB = invoke(property("anisotropyB"), "assign");
        const auto bitangentX = invoke(matrix.get(JSValue(1.0)), "mul")({vector.get("x")});
        assignB({invoke(bitangentX, "sub")({invoke(matrix.get(JSValue(0.0)), "mul")({vector.get("y")})})});
    }
    if (self.get("useTransmission").toBoolean()) {
        const auto transmission = overrideNode(self, "transmissionNode", "float", "materialTransmission");
        const auto thickness = overrideNode(self, "thicknessNode", "float", "materialThickness");
        const auto distance = overrideNode(self, "attenuationDistanceNode", "float", "materialAttenuationDistance");
        const auto color = overrideNode(self, "attenuationColorNode", "vec3", "materialAttenuationColor");
        invoke(property("transmission"), "assign")({transmission});
        invoke(property("thickness"), "assign")({thickness});
        invoke(property("attenuationDistance"), "assign")({distance});
        invoke(property("attenuationColor"), "assign")({color});
        if (self.get("useDispersion").toBoolean()) {
            const auto value = overrideNode(self, "dispersionNode", "float", "materialDispersion");
            invoke(property("dispersion"), "assign")({value});
        }
    }
    return {};
}

JSValue setupClearcoatNormal(const JSValue& self, const Args&, const Args&) {
    return overrideNode(self, "clearcoatNormalNode", "vec3", "materialClearcoatNormal");
}
JSValue setup(const JSValue& self, const Args& args, const Args& captures) {
    const auto builder = arg(args, 0);
    auto context = builder.get("context");
    auto callback = JSValue::function(
        +[](const JSValue&, const Args&, const Args& c) -> JSValue {
            const auto normal = invoke(c.at(0), "setupClearcoatNormal")({c.at(1)});
            return nativeSubBuildNodeBinding("subBuild").call({}, {normal, "NORMAL", "vec3"});
        }, {self, builder}, "", 0);
    callback.setFunctionSourceText(MeshPhysicalNodeMaterialSource::clearcoatCallbackSource);
    context.set("setupClearcoatNormal", callback);
    superCall(self, captures, "setup", {builder});
    return {};
}
}

JSValue nativeMeshPhysicalNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshPhysicalNodeMaterial module");
    const auto base = nativeMeshStandardNodeMaterialConstructor();
    // Import-time singleton; keep it alive with the module, just like _defaultValues.
    const auto defaults = nativeMeshPhysicalMaterialConstructor().construct();
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "MeshPhysicalNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isMeshPhysicalNodeMaterial", true);
            for (const char* key : {"clearcoatNode", "clearcoatRoughnessNode", "diffuseRoughnessNode", "clearcoatNormalNode",
                 "sheenNode", "sheenRoughnessNode", "iridescenceNode", "iridescenceIORNode", "iridescenceThicknessNode",
                 "specularIntensityNode", "specularColorNode", "iorNode", "transmissionNode", "thicknessNode",
                 "attenuationDistanceNode", "attenuationColorNode", "dispersionNode", "retroreflectivityNode", "anisotropyNode"})
                self.set(key, nullptr);
            invoke(self, "setDefaultValues")({captures.at(1)});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        }, {defaults}, MeshPhysicalNodeMaterialSource::classSource, MeshPhysicalNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    const auto getter = [&](const char* key, const char* field, const char* overrideField, const char* source) {
        JSPropertyDescriptor d;
        auto fn = JSValue::function(useFeature, {field, overrideField}, std::string("get ") + key, 0);
        fn.setFunctionSourceText(source);
        d.getter = fn;
        d.configurable = true;
        prototype.defineProperty(key, d);
    };
    getter("useClearcoat", "clearcoat", "clearcoatNode", MeshPhysicalNodeMaterialSource::useClearcoatSource);
    getter("useDiffuseRoughness", "diffuseRoughness", "diffuseRoughnessNode", MeshPhysicalNodeMaterialSource::useDiffuseRoughnessSource);
    getter("useIridescence", "iridescence", "iridescenceNode", MeshPhysicalNodeMaterialSource::useIridescenceSource);
    getter("useSheen", "sheen", "sheenNode", MeshPhysicalNodeMaterialSource::useSheenSource);
    getter("useAnisotropy", "anisotropy", "anisotropyNode", MeshPhysicalNodeMaterialSource::useAnisotropySource);
    getter("useTransmission", "transmission", "transmissionNode", MeshPhysicalNodeMaterialSource::useTransmissionSource);
    getter("useDispersion", "dispersion", "dispersionNode", MeshPhysicalNodeMaterialSource::useDispersionSource);
    getter("useRetroreflection", "retroreflectivity", "retroreflectivityNode", MeshPhysicalNodeMaterialSource::useRetroreflectionSource);
    method(prototype, "setupSpecular", 0, setupSpecular, {}, MeshPhysicalNodeMaterialSource::setupSpecularSource);
    method(prototype, "setupLightingModel", 0, setupLightingModel, {}, MeshPhysicalNodeMaterialSource::setupLightingModelSource);
    method(prototype, "setupVariants", 1, setupVariants, {prototype}, MeshPhysicalNodeMaterialSource::setupVariantsSource);
    method(prototype, "setupClearcoatNormal", 0, setupClearcoatNormal, {}, MeshPhysicalNodeMaterialSource::setupClearcoatNormalSource);
    method(prototype, "setup", 1, setup, {prototype}, MeshPhysicalNodeMaterialSource::setupSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}
JSValue nativeMeshPhysicalNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshPhysicalNodeMaterialModule();
    (void)keeper;
    return lexical().lock()->get(name);
}
JSValue nativeMeshPhysicalNodeMaterialConstructor() { return nativeMeshPhysicalNodeMaterialBinding("default"); }
