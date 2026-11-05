#include "nodes/lighting/ProjectorLightNode.h"

#include <algorithm>

#include "math/JSNumberMath.h"
#include "nodes/accessors/Lights.h"
#include "nodes/accessors/Position.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/SpotLightNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}
JSWeakValue& sdBoxCache() {
    static thread_local JSWeakValue weak;
    return weak;
}

// const sdBox = Fn( ( [ p, b ] ) => { const d = p.abs().sub( b ); return length( max( d, 0.0 ) ).add( min( max( d.x, d.y ), 0.0 ) ); }, { p: 'vec2', b: 'vec2', return: 'float' } );
//
// The layout argument declares the source types for the TSL type system; this runtime keeps the
// callable and the layout is passed along as the source writes it.
JSValue buildSdBox() {
    if (const auto cached = sdBoxCache().lock(); cached && cached->isCallable()) return *cached;
    auto callback = JSValue::function(
        +[](const JSValue&, const Args& args, const Args&) -> JSValue {
            const auto p = arg(args, 0);
            const auto b = arg(args, 1);
            const auto zero = nativeTSLBaseBinding("float").call({}, {JSValue(0.0)});
            const auto lengthFn = nativeMathNodeBinding("length");
            const auto maxFn = nativeMathNodeBinding("max");
            const auto minFn = nativeMathNodeBinding("min");
            const auto d = invoke(invoke(p, "abs")(), "sub")({b});
            const auto outer = lengthFn.call({}, {maxFn.call({}, {d, zero})});
            const auto widest = maxFn.call({}, {d.get("x"), d.get("y")});
            const auto inner = minFn.call({}, {widest, zero});
            return invoke(outer, "add")({inner});
        },
        {}, "sdBox", 2);
    auto layout = JSValue::object(nullptr);
    layout.set("p", "vec2");
    layout.set("b", "vec2");
    layout.set("return", "float");
    const auto fn = nativeTSLCoreBinding("Fn").call({}, {callback, layout});
    sdBoxCache() = JSWeakValue(fn);
    return fn;
}

// update( frame ) { super.update( frame ); ... }
JSValue update(const JSValue& self, const Args& args, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};

    // super.update( frame ): the spot node's own update, called with this receiver.
    const auto spotPrototype = nativeSpotLightNodeBinding("default").get("prototype");
    spotPrototype.get("update").call(self, {arg(args, 0, nullptr)});

    // this.penumbraCosNode.value = Math.min( Math.cos( light.angle * ( 1 - light.penumbra ) ), .99999 );
    const auto angle = light.get("angle").toNumber();
    const auto penumbra = light.get("penumbra").toNumber();
    const auto coneCos = JSNumberMath::cos(angle * (1 - penumbra));
    // std::min matches Math.min for NaN (the first argument is returned) and for signed zero.
    self.get("penumbraCosNode").set("value", JSValue(std::min(coneCos, 0.99999)));

    auto shadow = light.get("shadow");
    if (light.get("aspect").isNull()) {
        double aspect = 1;
        const auto map = light.get("map");
        if (!map.isNull()) {
            aspect = map.get("width").toNumber() / map.get("height").toNumber();
        }
        shadow.set("aspect", JSValue(aspect));
    } else {
        shadow.set("aspect", light.get("aspect"));
    }
    return {};
}

// getSpotAttenuation( builder ) { ... If( spotLightCoord.w.greaterThan( 0 ), () => { ... } ); return attenuation; }
JSValue getSpotAttenuation(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto attenuation = nativeTSLBaseBinding("float").call({}, {JSValue(0.0)});
    const auto penumbraCos = self.get("penumbraCosNode");

    // const spotLightCoord = lightShadowMatrix( this.light ).mul( builder.context.positionWorld || positionWorld );
    const auto shadowMatrix = nativeLightsBinding("lightShadowMatrix").call({}, {self.get("light")});
    auto worldPosition = JSValue();
    if (builder.isObject()) {
        const auto context = builder.get("context");
        if (context.isObject()) worldPosition = context.get("positionWorld");
    }
    if (!worldPosition.toBoolean()) worldPosition = nativePositionBinding("positionWorld");
    const auto spotLightCoord = invoke(shadowMatrix, "mul")({worldPosition});

    // If( spotLightCoord.w.greaterThan( 0 ), () => { ... } )
    const auto condition = invoke(spotLightCoord.get("w"), "greaterThan")({JSValue(0.0)});
    auto body = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            const auto coord = captures[0];
            const auto penumbraCosNode = captures[1];
            const auto target = captures[2];
            // const projectionUV = spotLightCoord.xyz.div( spotLightCoord.w );
            const auto projectionUV =
                invoke(invoke(coord, "xyz")(), "div")({coord.get("w")});
            // const boxDist = sdBox( projectionUV.xy.sub( vec2( 0.5 ) ), vec2( 0.5 ) );
            const auto half = JSValue(0.5);
            const auto centered =
                invoke(projectionUV.get("xy"), "sub")({nativeTSLBaseBinding("vec2").call({}, {half})});
            const auto halfBox = nativeTSLBaseBinding("vec2").call({}, {half});
            const auto sdBox = buildSdBox();
            const auto boxDist = sdBox.call({}, {centered, halfBox});
            // const angleFactor = div( - 1.0, sub( 1.0, acos( penumbraCos ) ).sub( 1.0 ) );
            const auto sub = nativeOperatorNodeBinding("sub");
            const auto div = nativeOperatorNodeBinding("div");
            const auto acosNode = nativeMathNodeBinding("acos").call({}, {penumbraCosNode});
            const auto one = nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
            const auto denominator =
                invoke(sub.call({}, {one, acosNode}), "sub")({JSValue(1.0)});
            const auto angleFactor = div.call({}, {JSValue(-1.0), denominator});
            // attenuation.assign( saturate( boxDist.mul( - 2.0 ).mul( angleFactor ) ) );
            const auto scaled = invoke(invoke(boxDist, "mul")({JSValue(-2.0)}), "mul")({angleFactor});
            const auto saturated = nativeMathNodeBinding("saturate").call({}, {scaled});
            invoke(target, "assign")({saturated});
            return {};
        },
        {spotLightCoord, penumbraCos, attenuation}, "", 0);

    const auto If = nativeTSLCoreBinding("If");
    if (!If.isCallable()) {
        // The source has no fallback here; failing loudly is better than returning a wrong value.
        throw std::runtime_error("ProjectorLightNode: TSL If is unavailable in this runtime");
    }
    If.call({}, {condition, body});
    return attenuation;
}
}  // namespace

JSValue nativeProjectorLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ProjectorLightNode module");
    nativeTSLCore();
    nativeTSLBase();
    nativeLightsModule();
    nativePositionModule();
    nativeMathNodeModule();
    nativeOperatorNodeModule();
    const auto base = nativeSpotLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "ProjectorLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            return superConstruct(target, captures, {arg(args, 0, nullptr)});
        },
        {}, ProjectorLightNodeSource::classSource, ProjectorLightNodeSource::typeSource);
    method(ctor.get("prototype"), "update", 1, update, {}, ProjectorLightNodeSource::updateSource);
    method(ctor.get("prototype"), "getSpotAttenuation", 1, getSpotAttenuation, {},
           ProjectorLightNodeSource::getSpotAttenuationSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeProjectorLightNodeBinding(const std::string& name) {
    const auto keeper = nativeProjectorLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeProjectorLightNodeConstructor() {
    return nativeProjectorLightNodeBinding("default");
}

ProjectorLightNode::ProjectorLightNode(const JSValue& light)
    : object_(constructor().construct({light})) {}
