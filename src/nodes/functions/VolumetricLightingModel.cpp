#include "nodes/functions/VolumetricLightingModel.h"

#include <stdexcept>

#include "nodes/accessors/Camera.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/display/ViewportDepthNode.h"
#include "nodes/functions/BSDF/LTC.h"
#include "nodes/functions/NodeFunctionSources.inc"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/LoopNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <initializer_list>
#include <tuple>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSValue& baseKeeper() { static thread_local JSValue value; return value; }
// The module level property nodes live for the whole lifetime, so these are strong keepers.
JSValue& scatteringDensityKeeper() { static thread_local JSValue value; return value; }
JSValue& linearDepthRayKeeper() { static thread_local JSValue value; return value; }
JSValue& outgoingRayLightKeeper() { static thread_local JSValue value; return value; }

JSValue propertyNode(JSValue& keeper, const char* type) {
    if (!keeper.toBoolean()) keeper = nativeTSLBaseBinding("property").call({}, {JSValue(type)});
    return keeper;
}

JSValue baseInitialize(const JSValue& newTarget, const Args&, const Args&) {
    return JSValue::object(newTarget.get("prototype"));
}

// start sets the lights up and calls indirect; the remaining five hooks are abstract until overridden.
JSValue baseMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    if (captures[0].toString() == "start") {
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

// start( builder ) { ... }
//
// The ray marching loop of the source is NOT ported yet. The front to back decision below is transcribed,
// and the loop is where the ray is advanced and the scattering accumulated: that part needs LoopNode with
// its index callback plus the per step context writes, so it is left for its own round rather than
// half written. The method therefore throws instead of returning a model that would silently ignore the
// volume, which is what the project's rule asks for.
JSValue start(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto property = nativeTSLBaseBinding("property");
    const auto startPos = property.call({}, {JSValue("vec3")});
    const auto endPos = property.call({}, {JSValue("vec3")});
    const auto isFrontToBack = property.call({}, {JSValue("bool")});
    const auto cameraPosition = nativeCameraBinding("cameraPosition");
    const auto positionWorld = nativePositionBinding("positionWorld");
    const auto modelRadius = nativeModelNodeBinding("modelRadius");

    const auto distance = invoke(invoke(cameraPosition, "sub")({positionWorld}), "length")();
    const auto condition = invoke(distance, "greaterThan")({invoke(modelRadius, "mul")({JSValue(2.0)})});

    auto thenBranch = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            invoke(captures[0], "assign")({captures[2]});
            invoke(captures[1], "assign")({captures[3]});
            invoke(captures[4], "assign")({JSValue(true)});
            return {};
        },
        {startPos, endPos, cameraPosition, positionWorld, isFrontToBack}, "", 0);
    auto elseBranch = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            invoke(captures[0], "assign")({captures[3]});
            invoke(captures[1], "assign")({captures[2]});
            invoke(captures[4], "assign")({JSValue(false)});
            return {};
        },
        {startPos, endPos, cameraPosition, positionWorld, isFrontToBack}, "", 0);

    const auto If = nativeTSLBaseBinding("If");
    if (!If.isCallable()) throw std::runtime_error("VolumetricLightingModel: TSL If is unavailable");
    const auto statement = If.call({}, {condition, thenBranch});
    invoke(statement, "Else")({elseBranch});

    (void) builder;
    throw std::runtime_error(
        "VolumetricLightingModel::start: the ray marching loop is not ported yet");
}

// scatteringLight( lightColor, builder ) { ... }
JSValue scatteringLight(const JSValue&, const Args& args, const Args&) {
    const auto lightColor = arg(args, 0, nullptr);
    const auto builder = arg(args, 1, nullptr);
    const auto density = propertyNode(scatteringDensityKeeper(), "vec3");
    const auto sceneDepthNode = builder.get("context").get("sceneDepthNode");
    if (sceneDepthNode.toBoolean()) {
        const auto linearDepthRay = propertyNode(linearDepthRayKeeper(), "float");
        auto body = JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
                invoke(captures[0], "addAssign")({captures[1]});
                return {};
            },
            {density, lightColor}, "", 0);
        const auto condition = invoke(sceneDepthNode, "greaterThanEqual")({linearDepthRay});
        const auto If = nativeTSLBaseBinding("If");
        if (!If.isCallable()) {
            throw std::runtime_error("VolumetricLightingModel: TSL If is unavailable");
        }
        If.call({}, {condition, body});
        return {};
    }
    invoke(density, "addAssign")({lightColor});
    return {};
}

// direct( { lightNode, lightColor }, builder ) { ... }
JSValue direct(const JSValue& self, const Args& args, const Args&) {
    const auto inputs = arg(args, 0, nullptr);
    const auto builder = arg(args, 1, nullptr);
    const auto lightNode = inputs.get("lightNode");
    // if ( lightNode.isAnalyticLightNode !== true || lightNode.light.distance === undefined ) return;
    if (!lightNode.get("isAnalyticLightNode").strictEquals(true)) return {};
    if (lightNode.get("light").get("distance").isUndefined()) return {};

    auto directLight = invoke(invoke(inputs.get("lightColor"), "xyz")(), "toVar")();
    const auto shadowNode = lightNode.get("shadowNode");
    if (!shadowNode.isNull()) invoke(directLight, "mulAssign")({shadowNode});
    invoke(self, "scatteringLight")({directLight, builder});
    return {};
}

// directRectArea( { lightColor, lightPosition, halfWidth, halfHeight }, builder ) { ... }
JSValue directRectArea(const JSValue& self, const Args& args, const Args&) {
    const auto inputs = arg(args, 0, nullptr);
    const auto builder = arg(args, 1, nullptr);
    const auto lightColor = inputs.get("lightColor");
    const auto lightPosition = inputs.get("lightPosition");
    const auto halfWidth = inputs.get("halfWidth");
    const auto halfHeight = inputs.get("halfHeight");

    // counterclockwise; the light shines in local negative z direction
    const auto p0 = invoke(invoke(lightPosition, "add")({halfWidth}), "sub")({halfHeight});
    const auto p1 = invoke(invoke(lightPosition, "sub")({halfWidth}), "sub")({halfHeight});
    const auto p2 = invoke(invoke(lightPosition, "sub")({halfWidth}), "add")({halfHeight});
    const auto p3 = invoke(invoke(lightPosition, "add")({halfWidth}), "add")({halfHeight});
    const auto P = builder.get("context").get("positionView");

    auto evaluateInputs = JSValue::object(nullptr);
    evaluateInputs.set("P", P);
    evaluateInputs.set("p0", p0);
    evaluateInputs.set("p1", p1);
    evaluateInputs.set("p2", p2);
    evaluateInputs.set("p3", p3);
    const auto evaluated = nativeLTCBinding("LTC_Evaluate_Volume").call({}, {evaluateInputs});
    const auto directLight =
        invoke(invoke(invoke(lightColor, "xyz")(), "mul")({evaluated}), "pow")({JSValue(1.5)});
    invoke(self, "scatteringLight")({directLight, builder});
    return {};
}

// finish( builder ) { builder.context.outgoingLight.assign( outgoingRayLight ); }
JSValue finish(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    invoke(builder.get("context").get("outgoingLight"), "assign")(
        {propertyNode(outgoingRayLightKeeper(), "vec3")});
    return {};
}
}  // namespace

JSValue nativeVolumetricLightingModelModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "VolumetricLightingModel module");
    nativeCameraModule();
    nativePositionModule();
    nativeModelNodeModule();
    nativePropertyNodeModule();
    nativeTSLBase();
    nativeTSLCore();
    nativeLoopNodeModule();
    nativeViewportDepthNodeModule();
    nativeLTCModule();

    const auto base = makeLightingModelBase();
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args&, const Args& captures) -> JSValue {
            return captures[0].get("constructor").getPrototype().construct({}, target);
        },
        {holder}, "VolumetricLightingModel", 0);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    ctor.setFunctionSourceText(VolumetricLightingModelSource::classSource);

    for (const auto& [name, arity, callback, source] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {"start", 1, start, VolumetricLightingModelSource::startSource},
             {"scatteringLight", 2, scatteringLight,
              VolumetricLightingModelSource::scatteringLightSource},
             {"direct", 2, direct, VolumetricLightingModelSource::directSource},
             {"directRectArea", 2, directRectArea,
              VolumetricLightingModelSource::directRectAreaSource},
             {"finish", 1, finish, VolumetricLightingModelSource::finishSource}}) {
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

JSValue nativeVolumetricLightingModelBinding(const std::string& name) {
    const auto keeper = nativeVolumetricLightingModelModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeVolumetricLightingModelConstructor() {
    return nativeVolumetricLightingModelBinding("default");
}
