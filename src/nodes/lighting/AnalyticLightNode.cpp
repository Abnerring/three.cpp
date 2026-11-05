#include "nodes/lighting/AnalyticLightNode.h"

#include "lights/Light.h"
#include "math/Color.h"
#include "nodes/accessors/Lights.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/constants.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

JSValue update(const JSValue& self, const Args&, const Args&) {
    const auto light = self.get("light");
    if (light.isNull() || light.isUndefined()) return {};
    auto color = self.get("color");
    invoke(color, "copy")({light.get("color")});
    const auto intensity = light.get("intensity").toNumber();
    color.set("r", color.get("r").toNumber() * intensity);
    color.set("g", color.get("g").toNumber() * intensity);
    color.set("b", color.get("b").toNumber() * intensity);
    return {};
}

JSValue getHash(const JSValue& self, const Args&, const Args&) {
    return self.get("light").get("uuid");
}

// getLightVector( builder ) {
//
//     return lightViewPosition( this.light ).sub( builder.context.positionView || positionView );
//
// }
JSValue getLightVector(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto base = nativeLightsBinding("lightViewPosition").call({}, {self.get("light")});
    // builder.context.positionView || positionView: any truthy context value wins, exactly as the
    // source's short circuit does.
    auto positionView = JSValue();
    if (builder.isObject()) {
        const auto context = builder.get("context");
        if (context.isObject()) positionView = context.get("positionView");
    }
    if (!positionView.toBoolean()) positionView = nativePositionBinding("positionView");
    return invoke(base, "sub")({positionView});
}

JSValue setup(const JSValue&, const Args&, const Args&) { return {}; }
JSValue setupDirect(const JSValue&, const Args&, const Args&) { return {}; }
JSValue setupDirectRectArea(const JSValue&, const Args&, const Args&) { return {}; }
JSValue dispose(const JSValue&, const Args&, const Args&) { return {}; }
}

JSValue nativeAnalyticLightNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "AnalyticLightNode module");
    nativeLightsModule();
    nativePositionModule();
    const auto base = nativeLightingNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    // constructor( light = null ) has a default parameter, so Function.length is 0. The previous
    // value here was 1; the parity probe against the pinned module is what showed it.
    const auto ctor = makeClass(
        "AnalyticLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("light", arg(args, 0, nullptr));
            self.set("color", nativeColorConstructor().construct());
            auto colorNode = nativeUniformFactory().call({}, {self.get("color")});
            self.set("colorNode", invoke(colorNode, "setGroup")({nativeRenderGroup()}));
            self.set("baseColorNode", nullptr);
            self.set("shadowNode", nullptr);
            self.set("shadowColorNode", nullptr);
            self.set("isAnalyticLightNode", true);
            self.set("updateType", nativeNodeConstantsBinding("NodeUpdateType").get("FRAME"));
            return self;
        }, {}, AnalyticLightNodeSource::classSource, AnalyticLightNodeSource::typeSource);
    method(ctor.get("prototype"), "update", 1, update, {}, AnalyticLightNodeSource::updateSource);
    method(ctor.get("prototype"), "getHash", 0, getHash, {}, AnalyticLightNodeSource::getHashSource);
    method(ctor.get("prototype"), "getLightVector", 1, getLightVector, {},
           AnalyticLightNodeSource::getLightVectorSource);
    method(ctor.get("prototype"), "setup", 1, setup, {}, AnalyticLightNodeSource::setupSource);
    method(ctor.get("prototype"), "setupDirect", 1, setupDirect, {}, AnalyticLightNodeSource::setupDirectSource);
    method(ctor.get("prototype"), "setupDirectRectArea", 1, setupDirectRectArea, {}, AnalyticLightNodeSource::setupDirectRectAreaSource);
    method(ctor.get("prototype"), "dispose", 0, dispose, {}, AnalyticLightNodeSource::disposeSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeAnalyticLightNodeBinding(const std::string& name) {
    const auto keeper = nativeAnalyticLightNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeAnalyticLightNodeConstructor() { return nativeAnalyticLightNodeBinding("default"); }
AnalyticLightNode::AnalyticLightNode(const JSValue& light) : object_(constructor().construct({light})) {}
