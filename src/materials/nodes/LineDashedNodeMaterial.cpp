#include "materials/nodes/LineDashedNodeMaterial.h"

#include "materials/LineDashedMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/NodeMaterialSources.inc"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/core/AttributeNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _defaultValues = new LineDashedMaterial();  created once and reused.
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }

JSValue defaultValues() {
    if (const auto value = defaults().lock(); value && value->toBoolean()) return *value;
    const auto created = nativeLineDashedMaterialConstructor().construct();
    defaults() = JSWeakValue(created);
    return created;
}

// setupVariants( /* builder */ ) { ... }
JSValue setupVariants(const JSValue& self, const Args&, const Args&) {
    // Each of the four inputs is taken from the node override when present and from the material
    // accessor otherwise.
    const auto nodeOrDefault = [&](const char* nodeName, const char* accessor) -> JSValue {
        return self.get(nodeName).toBoolean()
            ? nativeTSLBaseBinding("float").call({}, {self.get(nodeName)})
            : nativeMaterialNodeBinding(accessor);
    };
    const auto offsetNode = nodeOrDefault("offsetNode", "materialLineDashOffset");
    const auto dashScaleNode = nodeOrDefault("dashScaleNode", "materialLineScale");
    const auto dashSizeNode = nodeOrDefault("dashSizeNode", "materialLineDashSize");
    const auto gapSizeNode = nodeOrDefault("gapSizeNode", "materialLineGapSize");

    const auto dashSizeProperty = nativePropertyNodeBinding("dashSize");
    const auto gapSizeProperty = nativePropertyNodeBinding("gapSize");
    invoke(dashSizeProperty, "assign")({dashSizeNode});
    invoke(gapSizeProperty, "assign")({gapSizeNode});

    // const vLineDistance = varying( attribute( 'lineDistance' ).mul( dashScaleNode ) );
    auto vLineDistance = nativeTSLBaseBinding("varying").call(
        {}, {invoke(nativeAttributeNodeBinding("attribute").call({}, {JSValue("lineDistance")}), "mul")(
                {dashScaleNode})});
    const auto vLineDistanceOffset =
        self.get("offsetNode").toBoolean() ? invoke(vLineDistance, "add")({offsetNode}) : vLineDistance;
    // vLineDistanceOffset.mod( dashSize.add( gapSize ) ).greaterThan( dashSize ).discard()
    const auto period = invoke(dashSizeProperty, "add")({gapSizeProperty});
    const auto remainder = invoke(vLineDistanceOffset, "mod")({period});
    const auto beyond = invoke(remainder, "greaterThan")({dashSizeProperty});
    invoke(beyond, "discard")();
    return {};
}
}  // namespace

JSValue nativeLineDashedNodeMaterialModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "LineDashedNodeMaterial module");
    nativeNodeMaterialModule();
    nativeAttributeNodeModule();
    nativePropertyNodeModule();
    nativeMaterialNodeModule();
    nativeTSLBase();
    const auto base = nativeNodeMaterialBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "LineDashedNodeMaterial", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("isLineDashedNodeMaterial", true);
            invoke(self, "setDefaultValues")({defaultValues()});
            self.set("dashOffset", 0);
            self.set("offsetNode", nullptr);
            self.set("dashScaleNode", nullptr);
            self.set("dashSizeNode", nullptr);
            self.set("gapSizeNode", nullptr);
            invoke(self, "setValues")({arg(args, 0)});
            return self;
        },
        {}, LineDashedNodeMaterialSource::classSource, LineDashedNodeMaterialSource::typeSource);
    // The source signature has its builder parameter commented out, so the length is zero.
    method(ctor.get("prototype"), "setupVariants", 0, setupVariants, {},
           LineDashedNodeMaterialSource::setupVariantsSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeLineDashedNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeLineDashedNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeLineDashedNodeMaterialConstructor() {
    return nativeLineDashedNodeMaterialBinding("default");
}
