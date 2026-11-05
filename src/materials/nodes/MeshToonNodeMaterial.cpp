#include "materials/nodes/MeshToonNodeMaterial.h"

#include "materials/MeshToonMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/functions/ToonLightingModel.h"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new MeshToonMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeMeshToonMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupLightingModel( /*builder*/ ) { return new ToonLightingModel(); }
JSValue setupLightingModel(const JSValue&, const Args&, const Args&) {
    return nativeToonLightingModelConstructor().construct();
}
}  // namespace

JSValue nativeMeshToonNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshToonNodeMaterial module");
    nativeNodeMaterialModule();
    nativeToonLightingModelModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "MeshToonNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isMeshToonNodeMaterial", true);
            self.set("lights", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, MeshToonNodeMaterialSource::classSource, MeshToonNodeMaterialSource::typeSource);
    method(ctor.get("prototype"), "setupLightingModel", 1, setupLightingModel, {},
           MeshToonNodeMaterialSource::setupLightingModelSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshToonNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshToonNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshToonNodeMaterialConstructor() {
    return nativeMeshToonNodeMaterialBinding("default");
}
