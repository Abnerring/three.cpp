#include "nodes/lighting/SpotLightNode.h"

#include "math/JSNumberMath.h"
#include "nodes/accessors/Lights.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/lighting/LightUtils.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

// A uniform over the given value, placed in the render group. The constructor builds five of
// these, which is why it is a helper rather than five copies.
JSValue renderUniform(const JSValue& value) {
    const auto uniform = nativeUniformFactory().call({}, {value});
    return invoke(uniform, "setGroup")({nativeRenderGroup()});
}

// update( frame ) {
//
//     super.update( frame );
//     const { light } = this;
//     this.coneCosNode.value = Math.cos( light.angle );
//     this.penumbraCosNode.value = Math.cos( light.angle * ( 1 - light.penumbra ) );
//     this.cutoffDistanceNode.value = light.distance;
//     this.decayExponentNode.value = light.decay;
//
// }
JSValue update(const JSValue& self, const Args&, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};

    // super.update( frame ): inlined the way this family does it.
    auto color = self.get("color");
    invoke(color, "copy")({light.get("color")});
    const auto intensity = light.get("intensity").toNumber();
    color.set("r", color.get("r").toNumber() * intensity);
    color.set("g", color.get("g").toNumber() * intensity);
    color.set("b", color.get("b").toNumber() * intensity);

    // Math.cos is the pinned V8 fdlibm implementation, not the platform libm.
    const auto angle = light.get("angle").toNumber();
    self.get("coneCosNode").set("value", JSValue(JSNumberMath::cos(angle)));
    const auto penumbra = light.get("penumbra").toNumber();
    self.get("penumbraCosNode").set("value", JSValue(JSNumberMath::cos(angle * (1 - penumbra))));
    self.get("cutoffDistanceNode").set("value", light.get("distance"));
    self.get("decayExponentNode").set("value", light.get("decay"));
    return {};
}

// getSpotAttenuation( builder, angleCosine ) {
//
//     const { coneCosNode, penumbraCosNode } = this;
//     return smoothstep( coneCosNode, penumbraCosNode, angleCosine );
//
// }
JSValue getSpotAttenuation(const JSValue& self, const Args& args, const Args&) {
    return nativeMathNodeBinding("smoothstep")
        .call({}, {self.get("coneCosNode"), self.get("penumbraCosNode"), arg(args, 1, nullptr)});
}

// getLightCoord( builder ) {
//
//     const properties = builder.getNodeProperties( this );
//     let projectionUV = properties.projectionUV;
//     if ( projectionUV === undefined ) {
//
//         projectionUV = lightProjectionUV( this.light, builder.context.positionWorld );
//         properties.projectionUV = projectionUV;
//
//     }
//     return projectionUV;
//
// }
JSValue getLightCoord(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    auto properties = invoke(builder, "getNodeProperties")({self});
    auto projectionUV = properties.get("projectionUV");
    if (projectionUV.isUndefined()) {
        projectionUV = nativeLightsBinding("lightProjectionUV")
                           .call({}, {self.get("light"), builder.get("context").get("positionWorld")});
        properties.set("projectionUV", projectionUV);
    }
    return projectionUV;
}

// setupDirect( builder ) { ... }  — reproduced step by step from the source.
JSValue setupDirect(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);

    const auto lightVector = invoke(self, "getLightVector")({builder});
    const auto lightDirection = invoke(lightVector, "normalize")();
    const auto targetDirection =
        nativeLightsBinding("lightTargetDirection").call({}, {self.get("light")});
    const auto angleCos = invoke(lightDirection, "dot")({targetDirection});
    const auto spotAttenuation = invoke(self, "getSpotAttenuation")({builder, angleCos});
    const auto lightDistance = invoke(lightVector, "length")();

    auto attenuationInputs = JSValue::object(nullptr);
    attenuationInputs.set("lightDistance", lightDistance);
    attenuationInputs.set("cutoffDistance", self.get("cutoffDistanceNode"));
    attenuationInputs.set("decayExponent", self.get("decayExponentNode"));
    const auto lightAttenuation = nativeGetDistanceAttenuation().call({}, {attenuationInputs});

    auto lightColor = invoke(self.get("colorNode"), "mul")({spotAttenuation});
    lightColor = invoke(lightColor, "mul")({lightAttenuation});

    const auto light = self.get("light");
    bool hasProjected = false;
    JSValue lightCoord;
    JSValue projected;

    if (light.get("colorNode").toBoolean()) {
        lightCoord = invoke(self, "getLightCoord")({builder});
        // light.colorNode( lightCoord )
        projected = light.get("colorNode").call(light, {lightCoord});
        hasProjected = true;
    } else if (light.get("map").toBoolean()) {
        lightCoord = invoke(self, "getLightCoord")({builder});
        const auto sampled =
            nativeTextureNodeBinding("texture").call({}, {light.get("map"), invoke(lightCoord, "xy")()});
        // .onRenderUpdate( () => light.map )
        auto callback = JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
                return captures[0];
            },
            {light.get("map")}, "", 0);
        projected = invoke(sampled, "onRenderUpdate")({callback});
        hasProjected = true;
    }

    if (hasProjected) {
        auto doubled = invoke(lightCoord, "mul")({JSValue(2.0)});
        auto shifted = invoke(doubled, "sub")({JSValue(1.0)});
        auto absolute = invoke(shifted, "abs")();
        auto lessThanOne = invoke(absolute, "lessThan")({JSValue(1.0)});
        const auto inSpotLightMap = invoke(lessThanOne, "all")();
        auto projectedColor = invoke(lightColor, "mul")({projected});
        lightColor = invoke(inSpotLightMap, "select")({projectedColor, lightColor});
    }

    auto result = JSValue::object(nullptr);
    result.set("lightColor", lightColor);
    result.set("lightDirection", lightDirection);
    return result;
}
}  // namespace

JSValue nativeSpotLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "SpotLightNode module");
    nativeAnalyticLightNodeModule();
    nativeLightsModule();
    nativeMathNodeModule();
    nativeTextureNodeModule();
    nativeLightUtilsModule();
    const auto base = nativeAnalyticLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "SpotLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto self = superConstruct(target, captures, {light});
            self.set("coneCosNode", renderUniform(JSValue(0)));
            self.set("penumbraCosNode", renderUniform(JSValue(0)));
            self.set("cutoffDistanceNode", renderUniform(JSValue(0)));
            self.set("decayExponentNode", renderUniform(JSValue(0)));
            // The source replaces the parent's colour node with one over the same Color value.
            self.set("colorNode", renderUniform(self.get("color")));
            return self;
        },
        {}, SpotLightNodeSource::classSource, SpotLightNodeSource::typeSource);
    method(ctor.get("prototype"), "update", 1, update, {}, SpotLightNodeSource::updateSource);
    method(ctor.get("prototype"), "getSpotAttenuation", 2, getSpotAttenuation, {},
           SpotLightNodeSource::getSpotAttenuationSource);
    method(ctor.get("prototype"), "getLightCoord", 1, getLightCoord, {},
           SpotLightNodeSource::getLightCoordSource);
    method(ctor.get("prototype"), "setupDirect", 1, setupDirect, {},
           SpotLightNodeSource::setupDirectSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeSpotLightNodeBinding(const std::string& name) {
    const auto keeper = nativeSpotLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeSpotLightNodeConstructor() { return nativeSpotLightNodeBinding("default"); }

SpotLightNode::SpotLightNode(const JSValue& light) : object_(constructor().construct({light})) {}
