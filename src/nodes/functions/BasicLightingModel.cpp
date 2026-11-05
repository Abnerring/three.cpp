#include "nodes/functions/BasicLightingModel.h"

#include "constants.h"
#include "nodes/accessors/MaterialNode.h"
#include "nodes/core/LightingModel.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"

#include <algorithm>
#include <tuple>

namespace {

using namespace UtilityNodeRuntime;

#include "BasicLightingModelSource.inc"

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}

JSValue indirect(const JSValue&, const Args& args, const Args&) {
    const auto builderContext = arg(args, 0).get("context");
    const auto ambientOcclusion = builderContext.get("ambientOcclusion");
    const auto reflectedLight = builderContext.get("reflectedLight");
    const auto indirectDiffuse = reflectedLight.get("indirectDiffuse");
    const auto irradianceLightMap = builderContext.get("irradianceLightMap");
    const auto vec4 = nativeTSLBaseBinding("vec4");

    invoke(indirectDiffuse, "assign")({vec4.call({}, {0.0})});
    if (irradianceLightMap.toBoolean()) {
        invoke(indirectDiffuse, "addAssign")({irradianceLightMap});
    } else {
        invoke(indirectDiffuse, "addAssign")({vec4.call({}, {1.0, 1.0, 1.0, 0.0})});
    }
    invoke(indirectDiffuse, "mulAssign")({ambientOcclusion});
    invoke(indirectDiffuse, "mulAssign")({nativePropertyNodeBinding("diffuseColor").get("rgb")});
    return {};
}

JSValue finish(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto material = builder.get("material");
    const auto context = builder.get("context");
    const auto outgoingLight = context.get("outgoingLight");
    const auto envNode = context.get("environment");
    if (!envNode.toBoolean()) return {};

    const auto factor = invoke(nativeMaterialNodeBinding("materialSpecularStrength"), "mul")({
        nativeMaterialNodeBinding("materialReflectivity")});
    const auto outgoingRgb = outgoingLight.get("rgb");
    const auto envRgb = envNode.get("rgb");
    const auto combine = material.get("combine").toNumber();
    if (combine == MultiplyOperation) {
        const auto mixed = nativeMathNodeBinding("mix").call({}, {
            outgoingRgb,
            invoke(outgoingRgb, "mul")({envRgb}),
            factor});
        invoke(outgoingRgb, "assign")({mixed});
    } else if (combine == MixOperation) {
        const auto mixed = nativeMathNodeBinding("mix").call({}, {outgoingRgb, envRgb, factor});
        invoke(outgoingRgb, "assign")({mixed});
    } else if (combine == AddOperation) {
        invoke(outgoingRgb, "addAssign")({invoke(envRgb, "mul")({factor})});
    } else {
        ThreeUtils::warn({"BasicLightingModel: Unsupported .combine value:", material.get("combine")});
    }
    return {};
}

} // namespace

JSValue nativeBasicLightingModelModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "BasicLightingModel module");

    // Resolve imports in the same order as the pinned module. The values are
    // retained by the native callback graph through the module bindings.
    nativePropertyNodeModule();
    nativeMaterialNodeModule();
    nativeMathNodeModule();
    nativeTSLBase();

    const auto base = nativeLightingModelConstructor();
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args&, const Args& captures) -> JSValue {
            return captures[0].get("constructor").getPrototype().construct({}, target);
        },
        {holder}, "BasicLightingModel", 0);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(BasicLightingModelSource::classSource);

    for (const auto& [name, arity, callback, source] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {"indirect", 1, indirect, BasicLightingModelSource::indirectSource},
             {"finish", 1, finish, BasicLightingModelSource::finishSource}}) {
        auto fn = JSValue::function(callback, {}, name, arity);
        fn.setFunctionSourceText(source);
        JSPropertyDescriptor descriptor;
        descriptor.value = fn;
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    auto module = JSValue::object(nullptr);
    own(module, "default", ctor);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeBasicLightingModelBinding(const std::string& name) {
    const auto keeper = nativeBasicLightingModelModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeBasicLightingModel() {
    return nativeBasicLightingModelBinding("default");
}

BasicLightingModel::BasicLightingModel() : object_(constructor().construct()) {}
