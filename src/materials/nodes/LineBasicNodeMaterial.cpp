#include "materials/nodes/LineBasicNodeMaterial.h"

#include "materials/LineBasicMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new LineBasicMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeLineBasicMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}
}  // namespace

JSValue nativeLineBasicNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "LineBasicNodeMaterial module");
    nativeNodeMaterialModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "LineBasicNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isLineBasicNodeMaterial", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, LineBasicNodeMaterialSource::classSource, LineBasicNodeMaterialSource::typeSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeLineBasicNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeLineBasicNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeLineBasicNodeMaterialConstructor() {
    return nativeLineBasicNodeMaterialBinding("default");
}