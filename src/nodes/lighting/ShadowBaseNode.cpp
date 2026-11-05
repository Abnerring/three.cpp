#include "nodes/lighting/ShadowBaseNode.h"

#include "constants.h"
#include "core/JSWeakMap.h"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/Node.h"
#include "nodes/lighting/LightingEventNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "renderers/common/ChainMap.h"

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _shadowMaterialLib = new WeakMap()
JSWeakValue& materialLibrary() { static thread_local JSWeakValue weak; return weak; }
// const _shadowRenderObjectLibrary = new ChainMap()
JSWeakValue& renderObjectLibrary() { static thread_local JSWeakValue weak; return weak; }
// const _shadowRenderObjectKeys = []
JSWeakValue& renderObjectKeys() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& positionWorldProperty() { static thread_local JSWeakValue weak; return weak; }

JSValue shadowPositionWorldNode() {
    if (const auto cached = positionWorldProperty().lock(); cached && cached->toBoolean()) {
        return *cached;
    }
    const auto value = nativeTSLBaseBinding("property").call(
        {}, {JSValue("vec3"), JSValue("shadowPositionWorld")});
    positionWorldProperty() = JSWeakValue(value);
    return value;
}

JSValue materialLibraryObject() {
    if (const auto cached = materialLibrary().lock(); cached && cached->toBoolean()) return *cached;
    const auto created = createNativeWeakMap();
    materialLibrary() = JSWeakValue(created);
    return created;
}

JSValue renderObjectLibraryObject() {
    if (const auto cached = renderObjectLibrary().lock(); cached && cached->toBoolean()) {
        return *cached;
    }
    const auto created = nativeChainMapConstructor().construct();
    renderObjectLibrary() = JSWeakValue(created);
    return created;
}

JSValue renderObjectKeysArray() {
    if (const auto cached = renderObjectKeys().lock(); cached && cached->toBoolean()) return *cached;
    const auto created = JSValue::array(2);
    renderObjectKeys() = JSWeakValue(created);
    return created;
}

// const _getShadowMaterial = ( light ) => { let material = _shadowMaterialLib.get( light ); if ( material === undefined ) { ... } return material; }
JSValue getShadowMaterialFor(const JSValue& light) {
    JSWeakMap library(materialLibraryObject());
    auto material = library.get(light);
    if (material.isUndefined()) {
        auto created = nativeNodeMaterialConstructor().construct();
        created.set("colorNode", nativeTSLBaseBinding("vec4").call(
                                    {}, {JSValue(0), JSValue(0), JSValue(0), JSValue(1)}));
        // Use to avoid other overrideMaterial override material.colorNode unintentionally.
        created.set("isShadowPassMaterial", true);
        created.set("name", JSValue("ShadowMaterial"));
        created.set("blending", JSValue(static_cast<double>(NoBlending)));
        created.set("fog", false);
        library.set(light, created);
        material = created;
    }
    return material;
}

// const _disposeShadowMaterial = ( light ) => { const material = _shadowMaterialLib.get( light ); if ( material !== undefined ) { material.dispose(); _shadowMaterialLib.delete( light ); } }
void disposeShadowMaterialFor(const JSValue& light) {
    JSWeakMap library(materialLibraryObject());
    const auto material = library.get(light);
    if (!material.isUndefined()) {
        invoke(material, "dispose")();
        library.remove(light);
    }
}

// const _getShadowRenderObjectFunction = ( renderer, shadow, shadowType ) => { ... }
JSValue getShadowRenderObjectFunction(const JSValue& renderer, const JSValue& shadow,
                                      double shadowType) {
    auto keys = renderObjectKeysArray();
    keys.set("0", renderer);
    keys.set("1", shadow);

    const auto library = renderObjectLibraryObject();
    auto renderObjectFunction = invoke(library, "get")({keys});

    const bool stale = renderObjectFunction.isUndefined() ||
        !renderObjectFunction.get("shadowType").strictEquals(JSValue(shadowType));
    if (stale) {
        renderObjectFunction = JSValue::function(
            +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
                const auto object = arg(args, 0);
                const auto scene = arg(args, 1);
                const auto camera = arg(args, 2);
                const auto geometry = arg(args, 3);
                const auto material = arg(args, 4);
                const auto group = arg(args, 5);
                const auto lightsNode = arg(args, 6);
                const auto clippingContext = arg(args, 7);
                const auto passId = arg(args, 8);
                const auto rendererRef = captures[0];
                const auto shadowRef = captures[1];
                const auto type = captures[2].toNumber();

                const bool castsShadow = object.get("castShadow").strictEquals(true);
                const bool receivesUnderVsm = object.get("receiveShadow").toBoolean() &&
                    type == static_cast<double>(VSMShadowMap);
                if (!castsShadow && !receivesUnderVsm) return {};

                const auto shadowCamera = shadowRef.get("camera");
                const auto overrideMaterial = scene.get("overrideMaterial");
                invoke(object, "onBeforeShadow")({rendererRef, object, camera, shadowCamera, geometry,
                                                 overrideMaterial, group});
                invoke(rendererRef, "renderObject")({object, scene, camera, geometry, material, group,
                                                    lightsNode, clippingContext, passId});
                invoke(object, "onAfterShadow")({rendererRef, object, camera, shadowCamera, geometry,
                                                overrideMaterial, group});
                return {};
            },
            {renderer, shadow, JSValue(shadowType)}, "", 9);
        renderObjectFunction.set("shadowType", JSValue(shadowType));
        invoke(library, "set")({keys, renderObjectFunction});
    }

    keys.set("0", JSValue(nullptr));
    keys.set("1", JSValue(nullptr));
    return renderObjectFunction;
}

// isCacheable( /*builder*/ ) { return false; }
JSValue isCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// getShadowMaterial() { return _getShadowMaterial( this.light ); }
JSValue getShadowMaterial(const JSValue& self, const Args&, const Args&) {
    return getShadowMaterialFor(self.get("light"));
}

// disposeShadowMaterial() { _disposeShadowMaterial( this.light ); }
JSValue disposeShadowMaterial(const JSValue& self, const Args&, const Args&) {
    disposeShadowMaterialFor(self.get("light"));
    return {};
}

// getShadowRenderObjectFunction( renderer, shadow = this.light.shadow ) { return _getShadowRenderObjectFunction( renderer, shadow, renderer.shadowMap.type ); }
JSValue getShadowRenderObjectFunctionMethod(const JSValue& self, const Args& args, const Args&) {
    const auto renderer = arg(args, 0, nullptr);
    auto shadow = arg(args, 1, JSValue{});
    if (shadow.isUndefined()) shadow = self.get("light").get("shadow");
    return getShadowRenderObjectFunction(renderer, shadow,
                                         renderer.get("shadowMap").get("type").toNumber());
}

// setupShadowPosition( { context, material } ) { shadowPositionWorld.assign( material.receivedShadowPositionNode || context.shadowPositionWorld || positionWorld ); }
JSValue setupShadowPosition(const JSValue&, const Args& args, const Args&) {
    const auto configuration = arg(args, 0);
    const auto context = configuration.get("context");
    const auto material = configuration.get("material");
    auto target = material.isObject() ? material.get("receivedShadowPositionNode") : JSValue{};
    if (!target.toBoolean()) target = context.isObject() ? context.get("shadowPositionWorld") : JSValue{};
    if (!target.toBoolean()) target = nativePositionBinding("positionWorld");
    invoke(shadowPositionWorldNode(), "assign")({target});
    return {};
}
}  // namespace

JSValue nativeShadowBaseNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ShadowBaseNode module");
    nativeNodeMaterialModule();
    nativeChainMapModule();
    nativeTSLBase();
    nativePositionModule();
    const auto base = nativeNodeConstructor();
    auto bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "ShadowBaseNode", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            self.set("light", arg(args, 0, nullptr));
            // Shadows are updated by default per render.
            self.set("updateBeforeType", nativeNodeConstantsBinding("NodeUpdateType").get("RENDER"));
            self.set("isShadowBaseNode", true);
            return self;
        },
        {}, ShadowBaseNodeSource::classSource, ShadowBaseNodeSource::typeSource);
    const auto prototype = ctor.get("prototype");
    method(prototype, "isCacheable", 1, isCacheable, {}, ShadowBaseNodeSource::isCacheableSource);
    method(prototype, "getShadowMaterial", 0, getShadowMaterial, {},
           ShadowBaseNodeSource::getShadowMaterialSource);
    method(prototype, "disposeShadowMaterial", 0, disposeShadowMaterial, {},
           ShadowBaseNodeSource::disposeShadowMaterialSource);
    method(prototype, "getShadowRenderObjectFunction", 2, getShadowRenderObjectFunctionMethod, {},
           ShadowBaseNodeSource::getShadowRenderObjectFunctionSource);
    method(prototype, "setupShadowPosition", 1, setupShadowPosition, {},
           ShadowBaseNodeSource::setupShadowPositionSource);
    own(bindings, "shadowPositionWorld", shadowPositionWorldNode());
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeShadowBaseNodeBinding(const std::string& name) {
    const auto keeper = nativeShadowBaseNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeShadowBaseNodeConstructor() { return nativeShadowBaseNodeBinding("default"); }
JSValue nativeShadowPositionWorld() { return nativeShadowBaseNodeBinding("shadowPositionWorld"); }

ShadowBaseNode::ShadowBaseNode(const JSValue& light)
    : object_(constructor().construct({light})) {}
