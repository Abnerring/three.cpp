#include "nodes/lighting/PointLightNode.h"

#include <stdexcept>

#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/lighting/LightUtils.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue renderUniform(const JSValue& value) {
    const auto uniform = nativeUniformFactory().call({}, {value});
    return invoke(uniform, "setGroup")({nativeRenderGroup()});
}

// export const directPointLight = ( { color, lightVector, cutoffDistance, decayExponent } ) => { ... }
JSValue directPointLight(const JSValue&, const Args& args, const Args&) {
    const auto inputs = arg(args, 0);
    const auto color = inputs.get("color");
    const auto lightVector = inputs.get("lightVector");

    // The direction and the distance both come from the same vector, in this order.
    const auto lightDirection = invoke(lightVector, "normalize")();
    const auto lightDistance = invoke(lightVector, "length")();

    auto attenuationInputs = JSValue::object(nullptr);
    attenuationInputs.set("lightDistance", lightDistance);
    attenuationInputs.set("cutoffDistance", inputs.get("cutoffDistance"));
    attenuationInputs.set("decayExponent", inputs.get("decayExponent"));
    const auto attenuation = nativeGetDistanceAttenuation().call({}, {attenuationInputs});

    auto result = JSValue::object(nullptr);
    result.set("lightDirection", lightDirection);
    result.set("lightColor", invoke(color, "mul")({attenuation}));
    return result;
}

// update( frame ) { super.update( frame ); this.cutoffDistanceNode.value = light.distance; this.decayExponentNode.value = light.decay; }
JSValue update(const JSValue& self, const Args&, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};

    // super.update( frame ), inlined as this family does it.
    auto color = self.get("color");
    invoke(color, "copy")({light.get("color")});
    const auto intensity = light.get("intensity").toNumber();
    color.set("r", color.get("r").toNumber() * intensity);
    color.set("g", color.get("g").toNumber() * intensity);
    color.set("b", color.get("b").toNumber() * intensity);

    self.get("cutoffDistanceNode").set("value", light.get("distance"));
    self.get("decayExponentNode").set("value", light.get("decay"));
    return {};
}

// setupShadowNode() { return pointShadow( this.light ); }
JSValue setupShadowNode(const JSValue&, const Args&, const Args&) {
    // The module this delegates to, src/nodes/lighting/PointShadowNode.js, is not ported yet. The
    // project requires an explicit error instead of a value that would silently be wrong, so this
    // method says exactly what is missing rather than returning undefined.
    throw std::runtime_error(
        "PointLightNode::setupShadowNode: pointShadow is not available because "
        "src/nodes/lighting/PointShadowNode.js is not ported");
}

// setupDirect( builder ) { return directPointLight( { color: this.colorNode, lightVector: this.getLightVector( builder ), cutoffDistance: this.cutoffDistanceNode, decayExponent: this.decayExponentNode } ); }
JSValue setupDirect(const JSValue& self, const Args& args, const Args&) {
    auto inputs = JSValue::object(nullptr);
    inputs.set("color", self.get("colorNode"));
    inputs.set("lightVector", invoke(self, "getLightVector")({arg(args, 0, nullptr)}));
    inputs.set("cutoffDistance", self.get("cutoffDistanceNode"));
    inputs.set("decayExponent", self.get("decayExponentNode"));
    return nativeDirectPointLight().call({}, {inputs});
}
}  // namespace

JSValue nativePointLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PointLightNode module");
    nativeLightUtilsModule();
    const auto base = nativeAnalyticLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "PointLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 0, nullptr)});
            self.set("cutoffDistanceNode", renderUniform(JSValue(0)));
            // The decay exponent starts at two, unlike the other uniforms here which start at zero.
            self.set("decayExponentNode", renderUniform(JSValue(2)));
            return self;
        },
        {}, PointLightNodeSource::classSource, PointLightNodeSource::typeSource);
    method(ctor.get("prototype"), "update", 1, update, {}, PointLightNodeSource::updateSource);
    method(ctor.get("prototype"), "setupShadowNode", 0, setupShadowNode, {},
           PointLightNodeSource::setupShadowNodeSource);
    method(ctor.get("prototype"), "setupDirect", 1, setupDirect, {},
           PointLightNodeSource::setupDirectSource);
    auto factory = JSValue::function(directPointLight, {}, "directPointLight", 1);
    factory.setFunctionSourceText(PointLightNodeSource::directPointLightSource);
    own(bindings, "directPointLight", factory);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativePointLightNodeBinding(const std::string& name) {
    const auto keeper = nativePointLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativePointLightNodeConstructor() { return nativePointLightNodeBinding("default"); }
JSValue nativeDirectPointLight() { return nativePointLightNodeBinding("directPointLight"); }

PointLightNode::PointLightNode(const JSValue& light) : object_(constructor().construct({light})) {}
