#include "nodes/lighting/AnalyticLightNode.h"

#include "math/Color.h"
#include "nodes/accessors/Lights.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/constants.h"
#include "nodes/lighting/AnalyticLightNodeSources.inc"
#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/ShadowNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

JSValue update(const JSValue& self, const Args&, const Args&) {
    const auto light = self.get("light");
    const auto color = invoke(self.get("color"), "copy")({light.get("color")});
    invoke(color, "multiplyScalar")({light.get("intensity")});
    return {};
}

JSValue getHash(const JSValue& self, const Args&, const Args&) {
    return self.get("light").get("uuid");
}

JSValue getLightVector(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto base = nativeLightsBinding("lightViewPosition").call({}, {self.get("light")});
    const auto sub = invoke(base, "sub");
    auto position = builder.get("context").get("positionView");
    if (!position.toBoolean()) position = nativePositionBinding("positionView");
    return sub({position});
}

JSValue setup(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    const auto builder = arg(args, 0);
    auto color = self.get("baseColorNode");
    if (!color.toBoolean()) color = self.get("colorNode");
    self.set("colorNode", color);

    if (self.get("light").get("castShadow").toBoolean()) {
        if (builder.get("object").get("receiveShadow").toBoolean()) {
            invoke(self, "setupShadow")({builder});
        }
    } else if (!self.get("shadowNode").isNull()) {
        invoke(self.get("shadowNode"), "dispose")();
        self.set("shadowNode", nullptr);
        self.set("shadowColorNode", nullptr);
    }

    const auto direct = invoke(self, "setupDirect")({builder});
    const auto rectArea = invoke(self, "setupDirectRectArea")({builder});
    if (direct.toBoolean()) {
        invoke(builder.get("lightsNode"), "setupDirectLight")({builder, self, direct});
    }
    if (rectArea.toBoolean()) {
        invoke(builder.get("lightsNode"), "setupDirectRectAreaLight")({builder, self, rectArea});
    }
    return {};
}

JSValue setupDirect(const JSValue&, const Args&, const Args&) { return {}; }
JSValue setupDirectRectArea(const JSValue&, const Args&, const Args&) { return {}; }

JSValue setupShadowNode(const JSValue& self, const Args&, const Args&) {
    return nativeShadowNodeBinding("shadow").call({}, {self.get("light")});
}

JSValue setupShadow(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    const auto builder = arg(args, 0);
    const auto renderer = builder.get("renderer");
    if (renderer.get("shadowMap").get("enabled").strictEquals(false)) return {};

    auto shadowColor = self.get("shadowColorNode");
    if (shadowColor.isNull()) {
        const auto custom = self.get("light").get("shadow").get("shadowNode");
        const auto shadow = !custom.isUndefined()
            ? nativeTSLCoreBinding("nodeObject").call({}, {custom})
            : invoke(self, "setupShadowNode")();
        self.set("shadowNode", shadow);
        shadowColor = invoke(self.get("colorNode"), "mul")({shadow});
        self.set("shadowColorNode", shadowColor);
        self.set("baseColorNode", self.get("colorNode"));
    }
    if (builder.get("context").get("getShadow").toBoolean()) {
        shadowColor = invoke(builder.get("context"), "getShadow")({self, builder});
    }
    self.set("colorNode", shadowColor);
    return {};
}

JSValue disposeShadow(const JSValue& receiver, const Args&, const Args&) {
    auto self = receiver;
    if (!self.get("shadowNode").isNull()) {
        invoke(self.get("shadowNode"), "dispose")();
        self.set("shadowNode", nullptr);
    }
    self.set("shadowColorNode", nullptr);
    if (!self.get("baseColorNode").isNull()) {
        self.set("colorNode", self.get("baseColorNode"));
        self.set("baseColorNode", nullptr);
    }
    return {};
}

JSValue dispose(const JSValue& self, const Args&, const Args& captures) {
    if (self.get("_shadowDisposeListener").toBoolean()) {
        invoke(self.get("light"), "removeEventListener")({"dispose", self.get("_shadowDisposeListener")});
    }
    captures[0].getPrototype().get("dispose", self).call(self);
    return {};
}
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
    const auto ctor = makeClass(
        "AnalyticLightNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            const auto light = arg(args, 0, nullptr);
            self.set("light", light);
            self.set("color", nativeColorConstructor().construct());
            auto colorNode = light.toBoolean() ? light.get("colorNode") : light;
            if (!colorNode.toBoolean()) {
                const auto uniform = nativeUniformFactory().call({}, {self.get("color")});
                colorNode = invoke(uniform, "setGroup")({nativeRenderGroup()});
            }
            self.set("colorNode", colorNode);
            self.set("baseColorNode", nullptr);
            self.set("shadowNode", nullptr);
            self.set("shadowColorNode", nullptr);
            self.set("isAnalyticLightNode", true);
            self.set("updateType", nativeNodeConstantsBinding("NodeUpdateType").get("FRAME"));
            if (light.toBoolean() && light.get("shadow").toBoolean()) {
                auto listener = JSValue::function(
                    +[](const JSValue&, const Args&, const Args& captured) -> JSValue {
                        invoke(captured[0], "disposeShadow")();
                        return {};
                    }, {self}, "", 0);
                listener.setFunctionSourceText(AnalyticLightNodeSource::shadowDisposeListenerSource);
                self.set("_shadowDisposeListener", listener);
                invoke(light, "addEventListener")({"dispose", self.get("_shadowDisposeListener")});
            }
            return self;
        }, {}, AnalyticLightNodeSource::classSource, AnalyticLightNodeSource::typeSource);
    const auto prototype = ctor.get("prototype");
    method(prototype, "dispose", 0, dispose, {prototype}, AnalyticLightNodeSource::disposeSource);
    method(prototype, "disposeShadow", 0, disposeShadow, {}, AnalyticLightNodeSource::disposeShadowSource);
    method(prototype, "getHash", 0, getHash, {}, AnalyticLightNodeSource::getHashSource);
    method(prototype, "getLightVector", 1, getLightVector, {},
           AnalyticLightNodeSource::getLightVectorSource);
    method(prototype, "setupDirect", 0, setupDirect, {}, AnalyticLightNodeSource::setupDirectSource);
    method(prototype, "setupDirectRectArea", 0, setupDirectRectArea, {}, AnalyticLightNodeSource::setupDirectRectAreaSource);
    method(prototype, "setupShadowNode", 0, setupShadowNode, {}, AnalyticLightNodeSource::setupShadowNodeSource);
    method(prototype, "setupShadow", 1, setupShadow, {}, AnalyticLightNodeSource::setupShadowSource);
    method(prototype, "setup", 1, setup, {}, AnalyticLightNodeSource::setupSource);
    method(prototype, "update", 0, update, {}, AnalyticLightNodeSource::updateSource);
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
