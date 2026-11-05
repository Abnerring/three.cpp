#include "nodes/lighting/HemisphereLightNode.h"

#include "math/Color.h"
#include "nodes/accessors/Lights.h"
#include "nodes/accessors/Normal.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/math/MathNode.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

// update( frame ) {
//
//     const { light } = this;
//     super.update( frame );
//     this.lightPositionNode.object3d = light;
//     this.groundColorNode.value.copy( light.groundColor ).multiplyScalar( light.intensity );
//
// }
//
// The parent body is inlined the way LightProbeNode does it: these native classes do not dispatch
// to a super method, so "super.update( frame )" is reproduced field by field in the same order.
// Colors are scaled component by component for the same reason the parent does it that way.
JSValue update(const JSValue& self, const Args&, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};

    // super.update( frame )
    auto color = self.get("color");
    invoke(color, "copy")({light.get("color")});
    const auto intensity = light.get("intensity").toNumber();
    color.set("r", color.get("r").toNumber() * intensity);
    color.set("g", color.get("g").toNumber() * intensity);
    color.set("b", color.get("b").toNumber() * intensity);

    // this.lightPositionNode.object3d = light;
    self.get("lightPositionNode").set("object3d", light);

    // this.groundColorNode.value.copy( light.groundColor ).multiplyScalar( light.intensity );
    auto ground = self.get("groundColorNode").get("value");
    invoke(ground, "copy")({light.get("groundColor")});
    ground.set("r", ground.get("r").toNumber() * intensity);
    ground.set("g", ground.get("g").toNumber() * intensity);
    ground.set("b", ground.get("b").toNumber() * intensity);
    return {};
}

// setup( builder ) {
//
//     const { colorNode, groundColorNode, lightDirectionNode } = this;
//     const dotNL = normalWorld.dot( lightDirectionNode );
//     const hemiDiffuseWeight = dotNL.mul( 0.5 ).add( 0.5 );
//     const irradiance = mix( groundColorNode, colorNode, hemiDiffuseWeight );
//     builder.context.irradiance.addAssign( irradiance );
//
// }
JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto normalWorld = nativeNormalBinding("normalWorld");
    const auto dotNL = invoke(normalWorld, "dot")({self.get("lightDirectionNode")});
    const auto weighted = invoke(dotNL, "mul")({JSValue(0.5)});
    const auto hemiDiffuseWeight = invoke(weighted, "add")({JSValue(0.5)});
    const auto irradiance = nativeMathNodeBinding("mix").call(
        {}, {self.get("groundColorNode"), self.get("colorNode"), hemiDiffuseWeight});
    invoke(builder.get("context").get("irradiance"), "addAssign")({irradiance});
    return {};
}
}  // namespace

JSValue nativeHemisphereLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "HemisphereLightNode module");
    nativeAnalyticLightNodeModule();
    nativeLightsModule();
    nativeNormalModule();
    nativeMathNodeModule();
    const auto base = nativeAnalyticLightNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "HemisphereLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto self = superConstruct(target, captures, {light});
            const auto lightPositionNode = nativeLightsBinding("lightPosition").call({}, {light});
            self.set("lightPositionNode", lightPositionNode);
            self.set("lightDirectionNode", invoke(lightPositionNode, "normalize")());
            const auto ground = nativeUniformFactory().call({}, {nativeColorConstructor().construct()});
            self.set("groundColorNode", invoke(ground, "setGroup")({nativeRenderGroup()}));
            return self;
        },
        {}, HemisphereLightNodeSource::classSource, HemisphereLightNodeSource::typeSource);
    method(ctor.get("prototype"), "update", 1, update, {}, HemisphereLightNodeSource::updateSource);
    method(ctor.get("prototype"), "setup", 1, setup, {}, HemisphereLightNodeSource::setupSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeHemisphereLightNodeBinding(const std::string& name) {
    const auto keeper = nativeHemisphereLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeHemisphereLightNodeConstructor() {
    return nativeHemisphereLightNodeBinding("default");
}

HemisphereLightNode::HemisphereLightNode(const JSValue& light)
    : object_(constructor().construct({light})) {}
