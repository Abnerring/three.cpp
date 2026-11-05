#include "nodes/lighting/EnvironmentNode.h"

#include "core/JSWeakMap.h"
#include "nodes/accessors/AccessorsUtils.h"
#include "nodes/accessors/Camera.h"
#include "nodes/accessors/MaterialProperties.h"
#include "nodes/accessors/Normal.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/IsolateNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/pmrem/PMREMNode.h"
#include "nodes/tsl/TSLBase.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _rendererCache = new WeakMap()
JSWeakValue& rendererCache() { static thread_local JSWeakValue weak; return weak; }

constexpr double kPi = 3.141592653589793;

JSValue rendererCacheObject() {
    if (const auto cached = rendererCache().lock(); cached && cached->toBoolean()) return *cached;
    const auto created = createNativeWeakMap();
    rendererCache() = JSWeakValue(created);
    return created;
}

// Calls one of the node accessor bindings, all of which follow the same factory shape.
JSValue accessor(const JSValue& table, const char* name) { return table.get(name); }

// const createRadianceContext = ( roughnessNode, normalViewNode ) => { let reflectVec = null; return { getUV: () => { ... }, getTextureLevel: () => roughnessNode }; }
JSValue createRadianceContext(const JSValue& roughnessNode, const JSValue& normalViewNode) {
    auto state = JSValue::object(nullptr);
    state.set("cached", nullptr);
    state.set("roughness", roughnessNode);
    state.set("normal", normalViewNode);

    const auto getUV = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            auto holder = captures[0];
            const auto cached = holder.get("cached");
            if (!cached.isNull()) return cached;

            // positionViewDirection.negate().reflect( normalViewNode )
            auto reflectVector = invoke(
                invoke(nativePositionBinding("positionViewDirection"), "negate")(),
                "reflect")({holder.get("normal")});
            // pow4( roughnessNode ).mix( reflectVec, normalViewNode ).normalize()
            reflectVector = invoke(
                invoke(nativeTSLBaseBinding("pow4").call({}, {holder.get("roughness")}),
                       "mix")({reflectVector, holder.get("normal")}),
                "normalize")();
            reflectVector = invoke(reflectVector, "transformDirection")(
                {nativeCameraBinding("cameraWorldMatrix")});
            holder.set("cached", reflectVector);
            return reflectVector;
        },
        {state}, "getUV", 0);

    const auto getTextureLevel = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            return captures[0].get("roughness");
        },
        {state}, "getTextureLevel", 0);

    auto context = JSValue::object(nullptr);
    context.set("getUV", getUV);
    context.set("getTextureLevel", getTextureLevel);
    return context;
}

// const createRetroRadianceContext = ( roughnessNode, normalViewNode ) => { ... }
JSValue createRetroRadianceContext(const JSValue& roughnessNode, const JSValue& normalViewNode) {
    auto state = JSValue::object(nullptr);
    state.set("cached", nullptr);
    state.set("roughness", roughnessNode);
    state.set("normal", normalViewNode);

    const auto getUV = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            auto holder = captures[0];
            const auto cached = holder.get("cached");
            if (!cached.isNull()) return cached;

            // pow4( roughnessNode ).mix( positionViewDirection, normalViewNode ).normalize()
            auto retroVector = invoke(
                invoke(nativeTSLBaseBinding("pow4").call({}, {holder.get("roughness")}),
                       "mix")({nativePositionBinding("positionViewDirection"), holder.get("normal")}),
                "normalize")();
            retroVector =
                invoke(retroVector, "transformDirection")({nativeCameraBinding("cameraWorldMatrix")});
            holder.set("cached", retroVector);
            return retroVector;
        },
        {state}, "getUV", 0);

    const auto getTextureLevel = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
            return captures[0].get("roughness");
        },
        {state}, "getTextureLevel", 0);

    auto context = JSValue::object(nullptr);
    context.set("getUV", getUV);
    context.set("getTextureLevel", getTextureLevel);
    return context;
}

// const createIrradianceContext = ( normalWorldNode ) => ( { getUV: () => normalWorldNode, getTextureLevel: () => float( 1.0 ) } )
JSValue createIrradianceContext(const JSValue& normalWorldNode) {
    const auto getUV = JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue { return captures[0]; },
        {normalWorldNode}, "getUV", 0);
    const auto getTextureLevel = JSValue::function(
        +[](const JSValue&, const Args&, const Args&) -> JSValue {
            return nativeTSLBaseBinding("float").call({}, {JSValue(1.0)});
        },
        {}, "getTextureLevel", 0);
    auto context = JSValue::object(nullptr);
    context.set("getUV", getUV);
    context.set("getTextureLevel", getTextureLevel);
    return context;
}

// _getPMREMNodeCache( renderer ) { let pmremCache = _rendererCache.get( renderer ); if ( pmremCache === undefined ) { pmremCache = new WeakMap(); _rendererCache.set( renderer, pmremCache ); } return pmremCache; }
JSValue getPMREMNodeCache(const JSValue&, const Args& args, const Args&) {
    JSWeakMap cache(rendererCacheObject());
    const auto renderer = arg(args, 0, nullptr);
    auto pmremCache = cache.get(renderer);
    if (pmremCache.isUndefined()) {
        pmremCache = createNativeWeakMap();
        cache.set(renderer, pmremCache);
    }
    return pmremCache;
}

// setup( builder ) { ... }
JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto material = builder.get("material");
    auto envNode = self.get("envNode");

    // if ( envNode.isTextureNode || envNode.isMaterialReferenceNode )
    if (envNode.get("isTextureNode").toBoolean() ||
        envNode.get("isMaterialReferenceNode").toBoolean()) {
        const bool isTexture = envNode.get("isTextureNode").toBoolean();
        const auto value = isTexture
            ? envNode.get("value")
            : material.get(envNode.get("property").string().c_str());

        const auto cache = invoke(self, "_getPMREMNodeCache")({builder.get("renderer")});
        auto cacheEnvNode = invoke(cache, "get")({value});
        if (cacheEnvNode.isUndefined()) {
            cacheEnvNode = nativePMREMNodeBinding("pmremTexture").call({}, {value});
            invoke(cache, "set")({value, cacheEnvNode});
        }
        envNode = cacheEnvNode;
    }

    const auto roughnessNode = nativePropertyNodeBinding("roughness");
    const auto materialEnvIntensity = nativeMaterialPropertiesBinding("materialEnvIntensity");

    const bool useAnisotropy = material.get("useAnisotropy").strictEquals(true) ||
        material.get("anisotropy").toNumber() > 0;
    const auto radianceNormalView = useAnisotropy
        ? nativeAccessorsUtilsBinding("bentNormalView")
        : nativeNormalBinding("normalView");

    auto radiance = nativeIsolateNodeBinding("isolate").call(
        {}, {invoke(invoke(envNode, "context")({createRadianceContext(roughnessNode, radianceNormalView)}),
                    "mul")({materialEnvIntensity})});

    if (material.get("useRetroreflection").strictEquals(true) ||
        material.get("retroreflectivity").toNumber() > 0) {
        const auto retroRadiance = nativeIsolateNodeBinding("isolate").call(
            {}, {invoke(invoke(envNode, "context")(
                            {createRetroRadianceContext(roughnessNode, radianceNormalView)}),
                        "mul")({materialEnvIntensity})});
        // retroreflectivity.clamp().mix( radiance, retroRadiance )
        radiance = invoke(invoke(nativePropertyNodeBinding("retroreflectivity"), "clamp")(),
                          "mix")({radiance, retroRadiance});
    }

    const auto irradiance =
        invoke(invoke(envNode, "context")({createIrradianceContext(nativeNormalBinding("normalWorld"))}),
               "mul")({JSValue(kPi)});
    const auto isolateIrradiance =
        nativeIsolateNodeBinding("isolate").call(
            {}, {invoke(irradiance, "mul")({materialEnvIntensity})});

    const auto builderContext = builder.get("context");
    invoke(builderContext.get("radiance"), "addAssign")({radiance});
    invoke(builderContext.get("iblIrradiance"), "addAssign")({isolateIrradiance});

    const auto clearcoatRadiance = builderContext.get("lightingModel").get("clearcoatRadiance");
    if (clearcoatRadiance.toBoolean()) {
        const auto clearcoatContext = invoke(envNode, "context")(
            {createRadianceContext(nativePropertyNodeBinding("clearcoatRoughness"),
                                   nativeNormalBinding("clearcoatNormalView"))});
        const auto isolateClearcoatRadiance = nativeIsolateNodeBinding("isolate").call(
            {}, {invoke(clearcoatContext, "mul")({materialEnvIntensity})});
        invoke(clearcoatRadiance, "addAssign")({isolateClearcoatRadiance});
    }
    return {};
}
}  // namespace

JSValue nativeEnvironmentNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "EnvironmentNode module");
    nativeLightingNodeModule();
    nativeTSLBase();
    nativeIsolateNodeModule();
    nativePropertyNodeModule();
    nativePMREMNodeModule();
    nativeNormalModule();
    nativeCameraModule();
    nativePositionModule();
    nativeAccessorsUtilsModule();
    nativeMaterialPropertiesModule();

    const auto base = nativeLightingNodeBinding("default");
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "EnvironmentNode", 0, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("envNode", arg(args, 0, nullptr));
            return self;
        },
        {}, EnvironmentNodeSource::classSource, EnvironmentNodeSource::typeSource);
    const auto prototype = ctor.get("prototype");
    method(prototype, "setup", 1, setup, {}, EnvironmentNodeSource::setupSource);
    method(prototype, "_getPMREMNodeCache", 1, getPMREMNodeCache, {},
           EnvironmentNodeSource::_getPMREMNodeCacheSource);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeEnvironmentNodeBinding(const std::string& name) {
    const auto keeper = nativeEnvironmentNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeEnvironmentNodeConstructor() { return nativeEnvironmentNodeBinding("default"); }

EnvironmentNode::EnvironmentNode(const JSValue& envNode)
    : object_(constructor().construct({envNode})) {}
