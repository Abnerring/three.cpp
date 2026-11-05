#include "nodes/lighting/LightsNode.h"

#include <algorithm>
#include <string>
#include <vector>

#include "core/JSWeakMap.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "utils.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// _lightsNodeRef: the per light cache of lighting nodes.
JSWeakValue& lightsNodeRef() { static thread_local JSWeakValue weak; return weak; }
// _hashData: the array reused across hash computations and always left empty.
JSWeakValue& hashDataArray() { static thread_local JSWeakValue weak; return weak; }
// The three shared property nodes the class hands to every instance.
JSWeakValue& totalDiffuse() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& totalSpecular() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& outgoingLight() { static thread_local JSWeakValue weak; return weak; }

JSValue propertyNode(JSWeakValue& slot, const char* name) {
    if (const auto cached = slot.lock(); cached && cached->toBoolean()) return *cached;
    const auto created =
        nativeTSLBaseBinding("property").call({}, {JSValue("vec3"), JSValue(name)});
    slot = JSWeakValue(created);
    return created;
}

JSValue weakMapObject(JSWeakValue& slot) {
    if (const auto cached = slot.lock(); cached && cached->toBoolean()) return *cached;
    const auto created = createNativeWeakMap();
    slot = JSWeakValue(created);
    return created;
}

JSValue sharedArray(JSWeakValue& slot, int length) {
    if (const auto cached = slot.lock(); cached && cached->toBoolean()) return *cached;
    const auto created = length < 0 ? JSValue::array() : JSValue::array(length);
    slot = JSWeakValue(created);
    return created;
}

std::string joinHashes(const JSValue& lightNodes) {
    std::string joined;
    const auto count = lightNodes.length();
    for (unsigned i = 0; i < count; ++i) {
        if (i > 0) joined += ",";
        joined += invoke(lightNodes.get(JSValue(static_cast<double>(i))), "getHash")().string();
    }
    return joined;
}

// getLightNodeById( id, lightNodes ): the first analytic light node whose light has that id.
JSValue getLightNodeById(double id, const JSValue& lightNodes) {
    const auto count = lightNodes.length();
    for (unsigned i = 0; i < count; ++i) {
        const auto candidate = lightNodes.get(JSValue(static_cast<double>(i)));
        if (!candidate.get("isAnalyticLightNode").toBoolean()) continue;
        if (candidate.get("light").get("id").toNumber() == id) return candidate;
    }
    return JSValue(nullptr);
}

void appendAll(JSValue& target, const JSValue& source) {
    if (!source.isObject() && !source.isCallable()) return;
    const auto count = source.length();
    for (unsigned i = 0; i < count; ++i) {
        target.set(target.length(), source.get(JSValue(static_cast<double>(i))));
    }
}

// isCacheable( /*builder*/ ) { return false; }
JSValue isCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// customCacheKey() { ... }
JSValue customCacheKey(const JSValue& self, const Args&, const Args&) {
    auto data = sharedArray(hashDataArray(), -1);
    data.set("length", JSValue(0));

    const auto builtinLights = invoke(self, "getBuiltinLights")();
    const auto count = builtinLights.length();
    for (unsigned i = 0; i < count; ++i) {
        const auto light = builtinLights.get(JSValue(static_cast<double>(i)));
        data.set(data.length(), light.get("id"));
        data.set(data.length(), JSValue(light.get("castShadow").toBoolean() ? 1.0 : 0.0));

        if (light.get("isSpotLight").strictEquals(true)) {
            const auto map = light.get("map");
            const auto hashMap = map.isNull() ? JSValue(-1) : map.get("id");
            const auto colorNode = light.get("colorNode");
            const auto hashColorNode =
                colorNode.toBoolean() ? invoke(colorNode, "getCacheKey")() : JSValue(-1);
            data.set(data.length(), hashMap);
            data.set(data.length(), hashColorNode);
        }
    }

    const auto cacheKey = nativeNodeUtilsBinding("hashArray").call({}, {data});
    data.set("length", JSValue(0));
    return cacheKey;
}

// getHash( builder ) { ... }
JSValue getHash(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    auto nodeData = invoke(builder, "getDataFromNode")({self});
    auto hash = nodeData.get("lightNodesHash");
    if (hash.isUndefined()) {
        const auto lightNodes = invoke(self, "setupLightsNode")({builder});
        nodeData.set("lightNodes", lightNodes);
        const auto joined = "lights-" + joinHashes(lightNodes);
        nodeData.set("lightNodesHash", JSValue(joined));
        hash = JSValue(joined);
    }
    return hash;
}

// setupLightsNode( builder ) { ... }
JSValue setupLightsNode(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    auto nodeData = invoke(builder, "getDataFromNode")({self});
    auto lightNodes = JSValue::array();

    auto previousLightNodes = nodeData.get("lightNodes");
    if (previousLightNodes.isUndefined()) previousLightNodes = JSValue(nullptr);
    const auto materialLightings = builder.get("context").get("materialLightings");

    auto lights = JSValue::array();
    appendAll(lights, materialLightings);
    appendAll(lights, invoke(self, "getBuiltinLights")());

    // sortLights: ascending by id. V8's sort is stable, so a stable sort here matches it.
    std::vector<JSValue> ordered;
    const auto lightCount = lights.length();
    for (unsigned i = 0; i < lightCount; ++i) ordered.push_back(lights.get(JSValue(static_cast<double>(i))));
    std::stable_sort(ordered.begin(), ordered.end(), [](const JSValue& a, const JSValue& b) {
        return a.get("id").toNumber() < b.get("id").toNumber();
    });

    for (const auto& light : ordered) {
        if (light.get("isNode").toBoolean()) {
            lightNodes.set(lightNodes.length(), light);
            continue;
        }

        JSValue lightNode;
        if (!previousLightNodes.isNull()) {
            lightNode = getLightNodeById(light.get("id").toNumber(), previousLightNodes);
        }
        if (lightNode.isNull()) {
            const auto lightNodeClass = light.get("_lightNode");
            if (lightNodeClass.isUndefined()) {
                ThreeUtils::warn({JSValue("LightsNode.setupNodeLights: Light node not found for " +
                                          light.get("constructor").get("name").string())});
                continue;
            }
            JSWeakMap refs(weakMapObject(lightsNodeRef()));
            if (!refs.has(light)) refs.set(light, lightNodeClass.construct({light}));
            lightNode = refs.get(light);
        }
        lightNodes.set(lightNodes.length(), lightNode);
    }
    return lightNodes;
}

// setupDirectLight( builder, lightNode, lightData ) { ... }
JSValue setupDirectLight(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto lightNode = arg(args, 1, nullptr);
    const auto lightData = arg(args, 2, nullptr);
    const auto context = builder.get("context");

    auto payload = JSValue::object(nullptr);
    for (const auto& key : lightData.ownPropertyNames()) payload.set(key.c_str(), lightData.get(key));
    payload.set("lightNode", lightNode);
    payload.set("reflectedLight", context.get("reflectedLight"));
    invoke(context.get("lightingModel"), "direct")({payload, builder});
    return {};
}

// setupDirectRectAreaLight( builder, lightNode, lightData ) { ... }
JSValue setupDirectRectAreaLight(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto lightNode = arg(args, 1, nullptr);
    const auto lightData = arg(args, 2, nullptr);
    const auto context = builder.get("context");

    auto payload = JSValue::object(nullptr);
    for (const auto& key : lightData.ownPropertyNames()) payload.set(key.c_str(), lightData.get(key));
    payload.set("lightNode", lightNode);
    payload.set("reflectedLight", context.get("reflectedLight"));
    invoke(context.get("lightingModel"), "directRectArea")({payload, builder});
    return {};
}

// setupLights( builder, lightNodes ) { for ( const lightNode of lightNodes ) lightNode.build( builder ); }
JSValue setupLights(const JSValue&, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto lightNodes = arg(args, 1, nullptr);
    const auto count = lightNodes.length();
    for (unsigned i = 0; i < count; ++i) {
        invoke(lightNodes.get(JSValue(static_cast<double>(i))), "build")({builder});
    }
    return {};
}

// getLightNodes( builder ) { ... }
JSValue getLightNodes(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    auto nodeData = invoke(builder, "getDataFromNode")({self});
    auto lightNodes = nodeData.get("lightNodes");
    if (lightNodes.isUndefined()) {
        lightNodes = invoke(self, "setupLightsNode")({builder});
        nodeData.set("lightNodes", lightNodes);
    }
    return lightNodes;
}

// setup( builder ) { ... }
JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto currentLightsNode = builder.get("lightsNode");
    auto builderCopy = builder;
    builderCopy.set("lightsNode", self);

    auto outgoingLightNode = self.get("outgoingLightNode");
    auto context = builder.get("context");
    const auto lightingModel = context.get("lightingModel");

    if (lightingModel.toBoolean()) {
        const auto totalDiffuseNode = self.get("totalDiffuseNode");
        const auto totalSpecularNode = self.get("totalSpecularNode");
        context.set("outgoingLight", outgoingLightNode);
        invoke(builder, "addStack")();
        invoke(lightingModel, "start")({builder});

        const auto backdrop = context.get("backdrop");
        const auto backdropAlpha = context.get("backdropAlpha");
        const auto reflected = context.get("reflectedLight");

        auto totalDiffuse =
            invoke(reflected.get("directDiffuse"), "add")({reflected.get("indirectDiffuse")});
        if (!backdrop.isNull()) {
            if (!backdropAlpha.isNull()) {
                totalDiffuse = nativeTSLBaseBinding("vec3").call(
                    {}, {invoke(backdropAlpha, "mix")({totalDiffuse, backdrop})});
            } else {
                totalDiffuse = nativeTSLBaseBinding("vec3").call({}, {backdrop});
            }
        }

        invoke(totalDiffuseNode, "assign")({totalDiffuse});
        invoke(totalSpecularNode, "assign")(
            {invoke(reflected.get("directSpecular"), "add")({reflected.get("indirectSpecular")})});
        invoke(outgoingLightNode, "assign")({invoke(totalDiffuseNode, "add")({totalSpecularNode})});
        invoke(lightingModel, "finish")({builder});
        outgoingLightNode = invoke(outgoingLightNode, "bypass")({invoke(builder, "removeStack")()});
    }

    builderCopy.set("lightsNode", currentLightsNode);
    return outgoingLightNode;
}

// setLights( lights ) { this._lights = lights; return this; }
JSValue setLights(const JSValue& self, const Args& args, const Args&) {
    auto selfRef = self;
    selfRef.set("_lights", arg(args, 0, nullptr));
    return self;
}

JSValue getLights(const JSValue& self, const Args&, const Args&) { return self.get("_lights"); }

// export const lights = ( lights = [] ) => new LightsNode().setLights( lights );
JSValue lightsFactory(const JSValue&, const Args& args, const Args& captures) {
    auto lightsValue = arg(args, 0, JSValue{});
    if (lightsValue.isUndefined()) lightsValue = JSValue::array();
    const auto node = captures[0].construct();
    return invoke(node, "setLights")({lightsValue});
}
}  // namespace

JSValue nativeLightsNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "LightsNode module");
    nativeTSLBase();
    nativeNodeUtils();

    const auto base = nativeNodeConstructor();
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "LightsNode", 0, base,
        +[](const JSValue& target, const Args&, const Args& captures) -> JSValue {
            // super( 'vec3' )
            auto self = superConstruct(target, captures, {JSValue("vec3")});
            self.set("totalDiffuseNode", propertyNode(totalDiffuse(), "totalDiffuse"));
            self.set("totalSpecularNode", propertyNode(totalSpecular(), "totalSpecular"));
            self.set("outgoingLightNode", propertyNode(outgoingLight(), "outgoingLight"));
            self.set("_lights", JSValue::array());
            self.set("global", true);
            return self;
        },
        {}, LightsNodeSource::classSource, LightsNodeSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "isCacheable", 1, isCacheable, {}, LightsNodeSource::isCacheableSource);
    method(prototype, "customCacheKey", 0, customCacheKey, {},
           LightsNodeSource::customCacheKeySource);
    method(prototype, "getHash", 1, getHash, {}, LightsNodeSource::getHashSource);
    method(prototype, "setupLightsNode", 1, setupLightsNode, {},
           LightsNodeSource::setupLightsNodeSource);
    method(prototype, "setupDirectLight", 3, setupDirectLight, {},
           LightsNodeSource::setupDirectLightSource);
    method(prototype, "setupDirectRectAreaLight", 3, setupDirectRectAreaLight, {},
           LightsNodeSource::setupDirectRectAreaLightSource);
    method(prototype, "setupLights", 2, setupLights, {}, LightsNodeSource::setupLightsSource);
    method(prototype, "getLightNodes", 1, getLightNodes, {}, LightsNodeSource::getLightNodesSource);
    method(prototype, "setup", 1, setup, {}, LightsNodeSource::setupSource);
    method(prototype, "setLights", 1, setLights, {}, LightsNodeSource::setLightsSource);
    method(prototype, "getLights", 0, getLights, {}, LightsNodeSource::getLightsSource);
    method(prototype, "getBuiltinLights", 0, getLights, {},
           LightsNodeSource::getBuiltinLightsSource);

    // get hasLights() { return this._lights.length > 0; }
    JSPropertyDescriptor hasLightsDescriptor;
    hasLightsDescriptor.configurable = true;
    hasLightsDescriptor.getter = JSValue::function(
        +[](const JSValue& self, const Args&, const Args&) -> JSValue {
            return JSValue(self.get("_lights").length() > 0);
        },
        {}, "get hasLights", 0);
    prototype.defineProperty("hasLights", hasLightsDescriptor);

    auto lightsFn = JSValue::function(lightsFactory, {ctor}, "lights", 0);
    own(bindings, "lights", lightsFn);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeLightsNodeBinding(const std::string& name) {
    const auto keeper = nativeLightsNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeLightsNodeConstructor() { return nativeLightsNodeBinding("default"); }
JSValue nativeLightsFactory() { return nativeLightsNodeBinding("lights"); }

LightsNode::LightsNode() : object_(constructor().construct()) {}
