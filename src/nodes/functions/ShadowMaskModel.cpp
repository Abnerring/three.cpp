#include "nodes/functions/ShadowMaskModel.h"

#include "nodes/core/PropertyNode.h"
#include "nodes/functions/NodeFunctionSources.inc"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <initializer_list>
#include <tuple>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue value;
    return value;
}
JSValue& baseKeeper() {
    static thread_local JSValue value;
    return value;
}

JSValue baseInitialize(const JSValue& newTarget, const Args&, const Args&) {
    return JSValue::object(newTarget.get("prototype"));
}

// The shared LightingModel surface: start sets the lights up and calls indirect, and the remaining five
// hooks are abstract extension points that answer nothing until a concrete model overrides them.
JSValue baseMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    const auto name = captures[0].toString();
    if (name == "start") {
        const auto builder = arg(args, 0);
        const auto lights = builder.get("lightsNode");
        invoke(lights, "setupLights")({builder, invoke(lights, "getLightNodes")({builder})});
        invoke(receiver, "indirect")({builder});
    }
    return {};
}

JSValue makeLightingModelBase() {
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(baseInitialize, {holder}, "LightingModel", 0);
    holder.set("constructor", ctor);
    auto prototype = ctor.get("prototype");
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             // Only start takes a named parameter in the source; the other five have their parameters
             // commented out, so their function length is zero.
             {"start", 1}, {"finish", 0}, {"direct", 0}, {"directRectArea", 0},
             {"indirect", 0}, {"ambientOcclusion", 0}}) {
        auto fn = JSValue::function(baseMethod, {name}, name, arity);
        JSPropertyDescriptor descriptor;
        descriptor.value = fn;
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }
    baseKeeper() = ctor;
    return ctor;
}

// direct( { lightNode } ) { if ( lightNode.shadowNode !== null ) this.shadowNode.mulAssign( lightNode.shadowNode ); }
JSValue direct(const JSValue& self, const Args& args, const Args&) {
    const auto lightNode = arg(args, 0).get("lightNode");
    if (!lightNode.get("shadowNode").isNull()) {
        invoke(self.get("shadowNode"), "mulAssign")({lightNode.get("shadowNode")});
    }
    return {};
}

// finish( { context } ) { diffuseColor.a.mulAssign( this.shadowNode.oneMinus() ); context.outgoingLight.rgb.assign( diffuseColor.rgb ); }
JSValue finish(const JSValue& self, const Args& args, const Args&) {
    const auto context = arg(args, 0).get("context");
    const auto diffuseColor = nativePropertyNodeBinding("diffuseColor");
    invoke(diffuseColor.get("a"), "mulAssign")({invoke(self.get("shadowNode"), "oneMinus")()});
    invoke(context.get("outgoingLight").get("rgb"), "assign")({diffuseColor.get("rgb")});
    return {};
}
}  // namespace

JSValue nativeShadowMaskModelModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ShadowMaskModel module");
    nativePropertyNodeModule();
    nativeTSLBase();

    const auto base = makeLightingModelBase();
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args&, const Args& captures) -> JSValue {
            auto self = captures[0].get("constructor").getPrototype().construct({}, target);
            // this.shadowNode = float( 1 ).toVar( 'shadowMask' )
            self.set("shadowNode", invoke(nativeTSLBaseBinding("float").call({}, {JSValue(1.0)}),
                                         "toVar")({JSValue("shadowMask")}));
            return self;
        },
        {holder}, "ShadowMaskModel", 0);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(ShadowMaskModelSource::classSource);

    for (const auto& [name, arity, callback, source] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {"direct", 1, direct, ShadowMaskModelSource::directSource},
             {"finish", 1, finish, ShadowMaskModelSource::finishSource}}) {
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

JSValue nativeShadowMaskModelBinding(const std::string& name) {
    const auto keeper = nativeShadowMaskModelModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeShadowMaskModelConstructor() { return nativeShadowMaskModelBinding("default"); }

ShadowMaskModel::ShadowMaskModel() : object_(constructor().construct()) {}
