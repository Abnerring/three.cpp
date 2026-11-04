#include "nodes/lighting/LightProbeNode.h"

#include "math/Vector3.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/UniformArrayNode.h"
#include "nodes/functions/material/getShIrradianceAt.h"
#include "nodes/lighting/AnalyticLightNode.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

JSValue update(const JSValue& self, const Args& args, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};
    auto color = self.get("color");
    invoke(color, "copy")({light.get("color")});
    const auto intensity = light.get("intensity").toNumber();
    color.set("r", color.get("r").toNumber() * intensity);
    color.set("g", color.get("g").toNumber() * intensity);
    color.set("b", color.get("b").toNumber() * intensity);
    const auto coefficients = light.get("sh").get("coefficients");
    const auto array = self.get("lightProbe").get("array");
    for (int i = 0; i < 9; ++i) {
        const auto target = array.get(JSValue(i));
        invoke(target, "copy")({coefficients.get(JSValue(i))});
        invoke(target, "multiplyScalar")({intensity});
    }
    return {};
}

JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto irradiance = nativeGetShIrradianceAt().call({}, {nativeNormalBinding("normalWorld"), self.get("lightProbe")});
    invoke(builder.get("context").get("irradiance"), "addAssign")({irradiance});
    return {};
}
}

JSValue nativeLightProbeNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "LightProbeNode module");
    nativeAnalyticLightNodeModule();
    nativeUniformArrayNodeModule();
    nativeNormalModule();
    auto bindings = JSValue::object(nullptr);
    const auto base = nativeAnalyticLightNodeBinding("default");
    const auto ctor = makeClass(
        "LightProbeNode", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 0, nullptr)});
            auto values = JSValue::array();
            const auto vector = nativeVector3Constructor();
            for (int i = 0; i < 9; ++i) values.set(JSValue(i), vector.construct());
            self.set("lightProbe", nativeUniformArrayNodeBinding("uniformArray").call({}, {values}));
            return self;
        }, {}, LightProbeNodeSource::classSource, LightProbeNodeSource::typeSource);
    method(ctor.get("prototype"), "update", 1, update, {}, LightProbeNodeSource::updateSource);
    method(ctor.get("prototype"), "setup", 1, setup, {}, LightProbeNodeSource::setupSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeLightProbeNodeBinding(const std::string& name) {
    const auto keeper = nativeLightProbeNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeLightProbeNodeConstructor() { return nativeLightProbeNodeBinding("default"); }
LightProbeNode::LightProbeNode(const JSValue& light) : object_(constructor().construct({light})) {}
