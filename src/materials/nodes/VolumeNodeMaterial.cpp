#include "materials/nodes/VolumeNodeMaterial.h"

#include "constants.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/functions/VolumetricLightingModel.h"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// setupLightingModel() { return new VolumetricLightingModel(); }
JSValue setupLightingModel(const JSValue&, const Args&, const Args&) {
    return nativeVolumetricLightingModelConstructor().construct();
}
}  // namespace

JSValue nativeVolumeNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "VolumeNodeMaterial module");
    nativeNodeMaterialModule();
    nativeVolumetricLightingModelModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "VolumeNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // This material has no default values object: every field below is written by the
            // constructor itself, before the parameters are applied.
            auto self = superConstruct(target, captures, {});
            self.set("isVolumeNodeMaterial", true);
            self.set("steps", 25);
            self.set("offsetNode", nullptr);
            self.set("scatteringNode", nullptr);
            self.set("lights", true);
            self.set("transparent", true);
            self.set("side", JSValue(static_cast<double>(BackSide)));
            self.set("depthTest", false);
            self.set("depthWrite", false);
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, VolumeNodeMaterialSource::classSource, VolumeNodeMaterialSource::typeSource);
    // The source declares setupLightingModel with no parameter at all, so its length is zero.
    method(ctor.get("prototype"), "setupLightingModel", 0, setupLightingModel, {},
           VolumeNodeMaterialSource::setupLightingModelSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeVolumeNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeVolumeNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeVolumeNodeMaterialConstructor() {
    return nativeVolumeNodeMaterialBinding("default");
}
