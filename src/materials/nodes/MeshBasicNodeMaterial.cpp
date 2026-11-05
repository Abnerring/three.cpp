#include "materials/nodes/MeshBasicNodeMaterial.h"

#include "materials/MeshBasicMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/display/FrontFacingNode.h"
#include "nodes/functions/BasicLightingModel.h"
#include "nodes/lighting/BasicEnvironmentNode.h"
#include "nodes/lighting/BasicLightMapNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new MeshBasicMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeMeshBasicMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupNormal() { return negateOnBackSide( normalViewGeometry ); }  // see #28839
JSValue setupNormal(const JSValue&, const Args&, const Args&) {
    return nativeFrontFacingNodeBinding("negateOnBackSide")
        .call({}, {nativeNormalBinding("normalViewGeometry")});
}

// setupEnvironment( builder ) { const envNode = super.setupEnvironment( builder ); return envNode ? new BasicEnvironmentNode( envNode ) : null; }
JSValue setupEnvironment(const JSValue& self, const Args& args, const Args&) {
    const auto basePrototype = nativeNodeMaterialBinding("default").get("prototype");
    const auto envNode = basePrototype.get("setupEnvironment").call(self, {arg(args, 0, nullptr)});
    if (!envNode.toBoolean()) return JSValue(nullptr);
    return nativeBasicEnvironmentNodeConstructor().construct({envNode});
}

// setupLightMap( builder ) { let node = null; if ( builder.material.lightMap ) { node = new BasicLightMapNode( materialLightMap ); } return node; }
JSValue setupLightMap(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    if (!builder.get("material").get("lightMap").toBoolean()) return JSValue(nullptr);
    return nativeBasicLightMapNodeConstructor().construct({nativeMaterialNodeBinding("materialLightMap")});
}

// setupOutgoingLight() { return diffuseColor.rgb; }
JSValue setupOutgoingLight(const JSValue&, const Args&, const Args&) {
    return nativePropertyNodeBinding("diffuseColor").get("rgb");
}

// setupLightingModel() { return new BasicLightingModel(); }  The binding is the class, so it has to be
// constructed here.
JSValue setupLightingModel(const JSValue&, const Args&, const Args&) {
    return nativeBasicLightingModel().construct();
}
}  // namespace

JSValue nativeMeshBasicNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshBasicNodeMaterial module");
    nativeNodeMaterialModule();
    nativeBasicEnvironmentNodeModule();
    nativeBasicLightMapNodeModule();
    nativeBasicLightingModelModule();
    nativeFrontFacingNodeModule();
    nativeNormalModule();
    nativePropertyNodeModule();
    nativeMaterialNodeModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "MeshBasicNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isMeshBasicNodeMaterial", true);
            self.set("lights", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, MeshBasicNodeMaterialSource::classSource, MeshBasicNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "setupNormal", 0, setupNormal, {}, MeshBasicNodeMaterialSource::setupNormalSource);
    method(prototype, "setupEnvironment", 1, setupEnvironment, {},
           MeshBasicNodeMaterialSource::setupEnvironmentSource);
    method(prototype, "setupLightMap", 1, setupLightMap, {},
           MeshBasicNodeMaterialSource::setupLightMapSource);
    method(prototype, "setupOutgoingLight", 0, setupOutgoingLight, {},
           MeshBasicNodeMaterialSource::setupOutgoingLightSource);
    method(prototype, "setupLightingModel", 0, setupLightingModel, {},
           MeshBasicNodeMaterialSource::setupLightingModelSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshBasicNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshBasicNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshBasicNodeMaterialConstructor() {
    return nativeMeshBasicNodeMaterialBinding("default");
}
