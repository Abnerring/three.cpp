#include "materials/nodes/MeshNormalNodeMaterial.h"

#include "constants.h"
#include "materials/MeshNormalMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/display/ColorSpaceNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/Packing.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new MeshNormalMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeMeshNormalMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupDiffuseColor() { const opacityNode = this.opacityNode ? float( this.opacityNode ) : materialOpacity; diffuseColor.assign( colorSpaceToWorking( vec4( packNormalToRGB( normalView ), opacityNode ), SRGBColorSpace ) ); }
JSValue setupDiffuseColor(const JSValue& self, const Args&, const Args&) {
    const auto opacityNode = self.get("opacityNode").toBoolean()
        ? nativeTSLBaseBinding("float").call({}, {self.get("opacityNode")})
        : nativeMaterialNodeBinding("materialOpacity");

    const auto packed =
        nativePackingBinding("packNormalToRGB").call({}, {nativeNormalBinding("normalView")});
    const auto colour = nativeTSLBaseBinding("vec4").call({}, {packed, opacityNode});
    // SRGBColorSpace is a string constant in this port.
    const auto converted = nativeColorSpaceNodeBinding("colorSpaceToWorking")
                               .call({}, {colour, JSValue(SRGBColorSpace)});
    invoke(nativePropertyNodeBinding("diffuseColor"), "assign")({converted});
    return {};
}
}  // namespace

JSValue nativeMeshNormalNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshNormalNodeMaterial module");
    nativeNodeMaterialModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "MeshNormalNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isMeshNormalNodeMaterial", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, MeshNormalNodeMaterialSource::classSource, MeshNormalNodeMaterialSource::typeSource);
    method(ctor.get("prototype"), "setupDiffuseColor", 0, setupDiffuseColor, {},
           MeshNormalNodeMaterialSource::setupDiffuseColorSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshNormalNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshNormalNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshNormalNodeMaterialConstructor() {
    return nativeMeshNormalNodeMaterialBinding("default");
}