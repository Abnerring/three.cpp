#include "materials/nodes/MeshStandardNodeMaterial.h"

#include "materials/MeshStandardMaterial.h"
#include "materials/nodes/MeshStandardNodeMaterialSources.inc"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/functions/PhysicalLightingModel.h"
#include "nodes/functions/material/getRoughness.h"
#include "nodes/lighting/EnvironmentNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

JSValue setupEnvironment(const JSValue& self, const Args& args, const Args& captures) {
    const auto builder = arg(args, 0);
    // super uses the method's home object and the original receiver, including
    // inherited accessors and changes to the home object's prototype.
    auto envNode = captures[0].getPrototype().get("setupEnvironment", self).call(self, {builder});
    if (envNode.isNull() && builder.get("environmentNode").toBoolean())
        envNode = builder.get("environmentNode");
    return envNode.toBoolean() ? nativeEnvironmentNodeConstructor().construct({envNode}) : JSValue(nullptr);
}

JSValue setupLightingModel(const JSValue&, const Args&, const Args&) {
    return nativePhysicalLightingModelConstructor().construct();
}

JSValue setupSpecular(const JSValue&, const Args&, const Args&) {
    const auto dielectric = nativeTSLBaseBinding("vec3").call({}, {0.04});
    const auto rgb = nativePropertyNodeBinding("diffuseColor").get("rgb");
    const auto specularColorNode = nativeMathNodeBinding("mix").call({},
        {dielectric, rgb, nativePropertyNodeBinding("metalness")});

    const auto assign = invoke(nativePropertyNodeBinding("specularColor"), "assign");
    assign({nativeTSLBaseBinding("vec3").call({}, {0.04})});
    invoke(nativePropertyNodeBinding("specularColorBlended"), "assign")({specularColorNode});
    invoke(nativePropertyNodeBinding("specularF90"), "assign")({1.0});
    return {};
}

JSValue setupVariants(const JSValue& self, const Args&, const Args&) {
    const auto metalnessNode = self.get("metalnessNode").toBoolean()
        ? nativeTSLBaseBinding("float").call({}, {self.get("metalnessNode")})
        : nativeMaterialNodeBinding("materialMetalness");
    invoke(nativePropertyNodeBinding("metalness"), "assign")({metalnessNode});

    auto roughnessNode = self.get("roughnessNode").toBoolean()
        ? nativeTSLBaseBinding("float").call({}, {self.get("roughnessNode")})
        : nativeMaterialNodeBinding("materialRoughness");
    auto inputs = JSValue::object();
    own(inputs, "roughness", roughnessNode);
    roughnessNode = nativeGetRoughness().call({}, {inputs});
    invoke(nativePropertyNodeBinding("roughness"), "assign")({roughnessNode});

    invoke(self, "setupSpecular")();

    const auto assign = invoke(nativePropertyNodeBinding("diffuseContribution"), "assign");
    const auto mul = invoke(nativePropertyNodeBinding("diffuseColor").get("rgb"), "mul");
    assign({mul({invoke(metalnessNode, "oneMinus")()})});
    return {};
}
} // namespace

JSValue nativeMeshStandardNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshStandardNodeMaterial module");
    const auto base = nativeNodeMaterialConstructor();
    nativeEnvironmentNodeModule();
    nativePhysicalLightingModelModule();
    nativeGetRoughnessModule();
    nativeMaterialNodeModule();
    nativePropertyNodeModule();
    nativeMathNodeModule();
    nativeTSLBase();
    // The source creates this single material at module evaluation. Capture it
    // in the constructor so GC and reentrant constructors retain its identity.
    const auto defaults = nativeMeshStandardMaterialConstructor().construct();
    const auto ctor = makeClass("MeshStandardNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("isMeshStandardNodeMaterial", true);
            self.set("lights", true);
            self.set("emissiveNode", nullptr);
            self.set("metalnessNode", nullptr);
            self.set("roughnessNode", nullptr);
            invoke(self, "setDefaultValues")({captures[1]});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        }, {defaults}, MeshStandardNodeMaterialSource::classSource, MeshStandardNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "setupEnvironment", 1, setupEnvironment, {prototype},
           MeshStandardNodeMaterialSource::setupEnvironmentSource);
    method(prototype, "setupLightingModel", 0, setupLightingModel, {},
           MeshStandardNodeMaterialSource::setupLightingModelSource);
    method(prototype, "setupSpecular", 0, setupSpecular, {}, MeshStandardNodeMaterialSource::setupSpecularSource);
    method(prototype, "setupVariants", 0, setupVariants, {}, MeshStandardNodeMaterialSource::setupVariantsSource);
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshStandardNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshStandardNodeMaterialModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshStandardNodeMaterialConstructor() {
    return nativeMeshStandardNodeMaterialBinding("default");
}
