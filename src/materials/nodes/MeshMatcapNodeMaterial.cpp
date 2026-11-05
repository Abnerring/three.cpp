#include "materials/nodes/MeshMatcapNodeMaterial.h"

#include "materials/MeshMatcapMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/accessors/MaterialReferenceNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/MatcapUV.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new MeshMatcapMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeMeshMatcapMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupVariants( builder ) { const uv = matcapUV; ... diffuseColor.rgb.mulAssign( matcapColor.rgb ); }
JSValue setupVariants(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto uv = nativeMatcapUV();
    const auto diffuseColor = nativePropertyNodeBinding("diffuseColor");

    if (builder.get("material").get("matcap").toBoolean()) {
        // materialReference( 'matcap', 'texture' ).context( { getUV: () => uv } )
        const auto reference = nativeMaterialReferenceNodeBinding("materialReference").call(
            {}, {JSValue("matcap"), JSValue("texture")});
        auto contextConfig = JSValue::object(nullptr);
        contextConfig.set("getUV", JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
                return captures[0];
            },
            {uv}, "getUV", 0));
        const auto matcapColor = invoke(reference, "context")({contextConfig});
        invoke(diffuseColor.get("rgb"), "mulAssign")({matcapColor.get("rgb")});
        return {};
    }

    // default if matcap is missing: vec3( mix( 0.2, 0.8, uv.y ) )
    const auto mixed = nativeMathNodeBinding("mix").call(
        {}, {JSValue(0.2), JSValue(0.8), uv.get("y")});
    const auto matcapColor = nativeTSLBaseBinding("vec3").call({}, {mixed});
    invoke(diffuseColor.get("rgb"), "mulAssign")({matcapColor.get("rgb")});
    return {};
}
}  // namespace

JSValue nativeMeshMatcapNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "MeshMatcapNodeMaterial module");
    nativeNodeMaterialModule();
    nativeMatcapUVModule();
    nativeMaterialReferenceNodeModule();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "MeshMatcapNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isMeshMatcapNodeMaterial", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, MeshMatcapNodeMaterialSource::classSource, MeshMatcapNodeMaterialSource::typeSource);
    method(ctor.get("prototype"), "setupVariants", 1, setupVariants, {},
           MeshMatcapNodeMaterialSource::setupVariantsSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeMeshMatcapNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeMeshMatcapNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeMeshMatcapNodeMaterialConstructor() {
    return nativeMeshMatcapNodeMaterialBinding("default");
}
