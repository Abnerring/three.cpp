#include "materials/nodes/MeshLambertNodeMaterial.h"

#include "materials/MeshLambertMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/functions/PhongLightingModel.h"
#include "nodes/lighting/BasicEnvironmentNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new MeshLambertMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeMeshLambertMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupEnvironment( builder ) { const envNode = super.setupEnvironment( builder ); return envNode ? new BasicEnvironmentNode( envNode ) : null; }
JSValue setupEnvironment(const JSValue& self, const Args& args, const Args&) {
    const auto basePrototype = nativeNodeMaterialBinding("default").get("prototype");
    const auto envNode = basePrototype.get("setupEnvironment").call(self, {arg(args, 0, nullptr)});
    if (!envNode.toBoolean()) return JSValue(nullptr);
    return nativeBasicEnvironmentNodeConstructor().construct({envNode});
}

// setupLightingModel( /*builder*/ ) { return new PhongLightingModel( false ); }  ( specular ) -> force lambert
JSValue setupLightingModel(const JSValue&, const Args&, const Args&) {
    return nativePhongLightingModelConstructor().construct({JSValue(false)});
}
}  // namespace

JSValue nativeMeshLambertNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshLambertNodeMaterial module");
    nativeNodeMaterialModule();
    nativeBasicEnvironmentNodeModule();
    nativePhongLightingModelModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "MeshLambertNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isMeshLambertNodeMaterial", true);
            self.set("lights", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, MeshLambertNodeMaterialSource::classSource, MeshLambertNodeMaterialSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "setupEnvironment", 1, setupEnvironment, {},
           MeshLambertNodeMaterialSource::setupEnvironmentSource);
    method(prototype, "setupLightingModel", 1, setupLightingModel, {},
           MeshLambertNodeMaterialSource::setupLightingModelSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshLambertNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshLambertNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshLambertNodeMaterialConstructor() {
    return nativeMeshLambertNodeMaterialBinding("default");
}
