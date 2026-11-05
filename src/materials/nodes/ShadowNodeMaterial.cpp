#include "materials/nodes/ShadowNodeMaterial.h"

#include "materials/ShadowMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/functions/ShadowMaskModel.h"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new ShadowMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeShadowMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupLightingModel( /*builder*/ ) { return new ShadowMaskModel(); }
JSValue setupLightingModel(const JSValue&, const Args&, const Args&) {
    return nativeShadowMaskModelConstructor().construct();
}
}  // namespace

JSValue nativeShadowNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ShadowNodeMaterial module");
    nativeNodeMaterialModule();
    nativeShadowMaskModelModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "ShadowNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isShadowNodeMaterial", true);
            self.set("lights", true);
            self.set("transparent", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, ShadowNodeMaterialSource::classSource, ShadowNodeMaterialSource::typeSource);
    method(ctor.get("prototype"), "setupLightingModel", 1, setupLightingModel, {},
           ShadowNodeMaterialSource::setupLightingModelSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeShadowNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeShadowNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeShadowNodeMaterialConstructor() {
    return nativeShadowNodeMaterialBinding("default");
}
