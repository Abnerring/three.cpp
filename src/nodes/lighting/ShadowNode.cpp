#include "nodes/lighting/ShadowNode.h"

#include "core/JSWeakMap.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/lighting/ShadowBaseNode.h"
#include "nodes/accessors/ReferenceNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/display/ViewportDepthNode.h"
#include "nodes/lighting/ShadowFilterNode.h"
#include "nodes/tsl/TSLBase.h"
#include "renderers/common/QuadMesh.h"
#include "renderers/common/RendererUtils.h"
#include "textures/DepthTexture.h"

#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// const _shadowFilterLib = [ BasicShadowFilter, PCFShadowFilter, null, VSMShadowFilter ]
JSValue shadowFilterLibrary() {
    auto library = JSValue::array(4);
    library.set(JSValue(0.0), nativeShadowFilterNodeBinding("BasicShadowFilter"));
    library.set(JSValue(1.0), nativeShadowFilterNodeBinding("PCFShadowFilter"));
    library.set(JSValue(2.0), JSValue(nullptr));
    library.set(JSValue(3.0), nativeShadowFilterNodeBinding("VSMShadowFilter"));
    return library;
}

// getShadowFilterFn( type ) { return _shadowFilterLib[ type ]; }
JSValue getShadowFilterFn(const JSValue&, const Args& args, const Args&) {
    return shadowFilterLibrary().get(arg(args, 0).toNumber());
}

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("ShadowNode: ") + name + " is not ported yet");
}

// dispose() { this._reset(); super.dispose(); }
JSValue disposeMethod(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto reset = self.get("_reset");
    if (reset.toBoolean()) reset.call(self, {});
    const auto basePrototype = nativeShadowBaseNodeConstructor().get("prototype");
    const auto baseDispose = basePrototype.get("dispose");
    if (baseDispose.toBoolean()) baseDispose.call(self, {});
    return {};
}

void disposeCache(JSValue& self, const char* name) {
    JSValue slot = self.get(name);
    if (!slot.toBoolean()) return;
    const auto dispose = slot.get("dispose");
    if (dispose.toBoolean()) dispose.call(slot, {});
    self.set(name, JSValue(nullptr));
}

// _reset() { ...dispose the cached shadow maps and VSM materials... }
JSValue resetMethod(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    self.set("_currentShadowType", JSValue(nullptr));
    if (const auto disposeShadowMaterial = self.get("disposeShadowMaterial");
        disposeShadowMaterial.toBoolean()) {
        disposeShadowMaterial.call(self, {});
    }
    disposeCache(self, "shadowMap");
    disposeCache(self, "vsmShadowMapVertical");
    disposeCache(self, "vsmShadowMapHorizontal");
    disposeCache(self, "vsmMaterialVertical");
    disposeCache(self, "vsmMaterialHorizontal");
    return {};
}

JSValue setupShadowFilter(const JSValue&, const Args&, const Args&) { return notPorted("setupShadowFilter"); }
// const bias = shadow.biasNode || reference( 'bias', 'float', shadow ).setGroup( renderGroup )
JSValue setupShadowCoord(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0);
    const auto shadowPosition = arg(args, 1);
    const auto shadow = self.get("shadow");
    const auto renderer = builder.get("renderer");
    const auto group = nativeUniformGroupNodeBinding("renderGroup");
    const auto referenceFn = nativeReferenceNodeBinding("reference");

    auto bias = shadow.get("biasNode");
    if (!bias.toBoolean()) {
        auto referenceNode =
            referenceFn.call({}, {JSValue("bias"), JSValue("float"), shadow});
        const auto setGroup = referenceNode.get("setGroup");
        bias = setGroup.call(referenceNode, {group});
    }

    auto shadowCoord = shadowPosition;
    JSValue coordZ;
    const auto camera = shadow.get("camera");
    const bool isOrthographic = camera.get("isOrthographicCamera").toBoolean();
    const bool logarithmicDepthBuffer = renderer.get("logarithmicDepthBuffer").toBoolean();
    if (isOrthographic || !logarithmicDepthBuffer) {
        const auto xyz = shadowCoord.get("xyz");
        const auto div = xyz.get("div");
        shadowCoord = div.call(xyz, {shadowCoord.get("w")});
        coordZ = shadowCoord.get("z");
    } else {
        const auto w = shadowCoord.get("w");
        const auto xy = shadowCoord.get("xy");
        const auto div = xy.get("div");
        shadowCoord = div.call(xy, {w});
        auto nearReference =
            referenceFn.call({}, {JSValue("near"), JSValue("float"), camera});
        nearReference = nearReference.get("setGroup").call(nearReference, {group});
        auto farReference = referenceFn.call({}, {JSValue("far"), JSValue("float"), camera});
        farReference = farReference.get("setGroup").call(farReference, {group});
        const auto negate = w.get("negate");
        const auto negatedW = negate.call(w, {});
        coordZ = nativeViewportDepthNodeBinding("viewZToLogarithmicDepth")
                     .call({}, {negatedW, nearReference, farReference});
    }

    const auto x = shadowCoord.get("x");
    const auto y = shadowCoord.get("y");
    const auto oneMinus = y.get("oneMinus");
    const auto yFlipped = oneMinus.call(y, {});
    JSValue z;
    if (renderer.get("reversedDepthBuffer").toBoolean()) {
        const auto sub = coordZ.get("sub");
        z = sub.call(coordZ, {bias});
    } else {
        const auto add = coordZ.get("add");
        z = add.call(coordZ, {bias});
    }
    return nativeTSLBaseBinding("vec3").call({}, {x, yFlipped, z});
}
// const depthTexture = new DepthTexture( shadow.mapSize.width, shadow.mapSize.height ) ...
JSValue setupRenderTarget(const JSValue&, const Args& args, const Args&) {
    const auto shadow = arg(args, 0);
    const auto builder = arg(args, 1);
    const auto mapSize = shadow.get("mapSize");
    const auto width = mapSize.get("width");
    const auto height = mapSize.get("height");
    const auto renderer = builder.get("renderer");

    auto depthTexture = nativeDepthTextureConstructor().construct({width, height});
    depthTexture.set("name", JSValue("ShadowDepthTexture"));
    const auto compareFunction = renderer.get("reversedDepthBuffer").toBoolean()
                                     ? nativeNodeConstantsBinding("GreaterEqualCompare")
                                     : nativeNodeConstantsBinding("LessEqualCompare");
    depthTexture.set("compareFunction", compareFunction);

    const auto createRenderTarget = builder.get("createRenderTarget");
    auto shadowMap = createRenderTarget.call(builder, {width, height});
    auto texture = shadowMap.get("texture");
    texture.set("name", JSValue("ShadowMap"));
    texture.set("type", shadow.get("mapType"));
    shadowMap.set("depthTexture", depthTexture);

    auto result = JSValue::object(nullptr);
    result.set("shadowMap", shadowMap);
    result.set("depthTexture", depthTexture);
    return result;
}
JSValue setupShadow(const JSValue&, const Args&, const Args&) { return notPorted("setupShadow"); }
JSValue setup(const JSValue&, const Args&, const Args&) { return notPorted("setup"); }
// renderShadow( frame ) { shadow.updateMatrices( light ); shadowMap.setSize(...); renderer.render( scene, shadow.camera ); }
JSValue renderShadow(const JSValue& self, const Args& args, const Args&) {
    const auto frame = arg(args, 0);
    const auto shadow = self.get("shadow");
    const auto shadowMap = self.get("shadowMap");
    const auto light = self.get("light");
    const auto renderer = frame.get("renderer");
    const auto scene = frame.get("scene");

    const auto updateMatrices = shadow.get("updateMatrices");
    updateMatrices.call(shadow, {light});
    const auto mapSize = shadow.get("mapSize");
    const auto setSize = shadowMap.get("setSize");
    setSize.call(shadowMap, {mapSize.get("width"), mapSize.get("height"), shadowMap.get("depth")});

    const auto currentSceneName = scene.get("name");
    const auto lightName = light.get("name");
    std::string lightLabel;
    if (lightName.toBoolean()) {
        lightLabel = lightName.toString();
    } else {
        lightLabel = "ID: " + std::to_string(static_cast<long long>(light.get("id").toNumber()));
    }
    JSValue& sceneRef = const_cast<JSValue&>(scene);
    sceneRef.set("name", JSValue("Shadow Map [ " + lightLabel + " ]"));
    renderer.get("render").call(renderer, {sceneRef, shadow.get("camera")});
    sceneRef.set("name", currentSceneName);
    return {};
}

// vsmPass( renderer ) { set the two VSM targets and render the shared quad mesh with each material. }
JSValue vsmPass(const JSValue& self, const Args& args, const Args&) {
    const auto renderer = arg(args, 0);
    const auto shadow = self.get("shadow");
    const auto shadowMap = self.get("shadowMap");
    const auto depth = shadowMap.get("depth");
    const auto mapSize = shadow.get("mapSize");
    const auto vertical = self.get("vsmShadowMapVertical");
    const auto horizontal = self.get("vsmShadowMapHorizontal");
    const auto setSizeVertical = vertical.get("setSize");
    setSizeVertical.call(vertical, {mapSize.get("width"), mapSize.get("height"), depth});
    const auto setSizeHorizontal = horizontal.get("setSize");
    setSizeHorizontal.call(horizontal, {mapSize.get("width"), mapSize.get("height"), depth});

    auto quadMesh = nativeQuadMeshConstructor().construct({JSValue(nullptr)});
    const auto setRenderTarget = renderer.get("setRenderTarget");
    setRenderTarget.call(renderer, {vertical});
    quadMesh.set("material", self.get("vsmMaterialVertical"));
    quadMesh.get("render").call(quadMesh, {renderer});
    setRenderTarget.call(renderer, {horizontal});
    quadMesh.set("material", self.get("vsmMaterialHorizontal"));
    quadMesh.get("render").call(quadMesh, {renderer});
    return {};
}
// module level renderer state cache, like the pinned `let _rendererState;`
JSWeakValue& rendererStateCache() { static thread_local JSWeakValue weak; return weak; }

// updateShadow( frame ) { save the renderer state, render the shadow map, restore it. }
JSValue updateShadow(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto frame = arg(args, 0);
    const auto shadowMap = self.get("shadowMap");
    const auto shadow = self.get("shadow");
    const auto renderer = frame.get("renderer");
    const auto scene = frame.get("scene");
    const auto camera = frame.get("camera");

    const auto shadowType = renderer.get("shadowMap").get("type");
    auto shadowCamera = shadow.get("camera");
    auto layers = shadowCamera.get("layers");
    const auto shadowCameraLayer = layers.get("mask").toNumber();
    // if ( ( shadow.camera.layers.mask & 0xFFFFFFFE ) === 0 ) shadow.camera.layers.mask = camera.layers.mask
    if ((static_cast<long long>(shadowCameraLayer) & 0xFFFFFFFEL) == 0) {
        layers.set("mask", camera.get("layers").get("mask"));
    }

    const auto currentRenderObjectFunction = renderer.get("getRenderObjectFunction").call(renderer, {});
    const auto resetState = nativeRendererUtilsBinding("resetRendererAndSceneState");
    auto rendererState = resetState.call({}, {renderer, scene, JSValue(nullptr)});
    rendererStateCache() = JSWeakValue(rendererState);

    JSValue& sceneRef = const_cast<JSValue&>(scene);
    sceneRef.set("overrideMaterial", self.get("getShadowMaterial").call(self, {}));
    renderer.get("setRenderObjectFunction")
        .call(renderer,
              {self.get("getShadowRenderObjectFunction").call(self, {renderer, shadow})});
    renderer.get("setClearColor").call(renderer, {JSValue(0x000000), JSValue(0)});
    renderer.get("setRenderTarget").call(renderer, {shadowMap});
    self.get("renderShadow").call(self, {frame});
    renderer.get("setRenderObjectFunction").call(renderer, {currentRenderObjectFunction});

    const auto vsmType = nativeNodeConstantsBinding("VSMShadowMap");
    if (shadowType.strictEquals(vsmType) && !shadow.get("isPointLightShadow").toBoolean()) {
        self.get("vsmPass").call(self, {renderer});
    }

    layers.set("mask", JSValue(shadowCameraLayer));
    nativeRendererUtilsBinding("restoreRendererAndSceneState")
        .call({}, {renderer, scene, rendererState});
    return {};
}
// updateBefore( frame ) { update the shadow map once per camera frame when needed. }
JSValue updateBefore(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto frame = arg(args, 0);
    const auto renderer = frame.get("renderer");
    // if ( frame.renderer._precompilationState !== null ) return;
    if (!renderer.get("_precompilationState").isNull()) return {};

    auto shadow = self.get("shadow");
    bool needsUpdate =
        shadow.get("needsUpdate").toBoolean() || shadow.get("autoUpdate").toBoolean();
    if (needsUpdate) {
        auto cameraFrameId = self.get("_cameraFrameId");
        const auto camera = frame.get("camera");
        const auto frameId = frame.get("frameId");
        const auto cached = cameraFrameId.get("get").call(cameraFrameId, {camera});
        if (cached.toBoolean() && cached.toNumber() == frameId.toNumber()) needsUpdate = false;
        cameraFrameId.get("set").call(cameraFrameId, {camera, frameId});
    }
    if (needsUpdate) {
        self.get("updateShadow").call(self, {frame});
        const_cast<JSValue&>(shadow).set("needsUpdate", false);
    }
    return {};
}
}  // namespace

JSValue nativeShadowNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ShadowNode module");
    nativeShadowBaseNodeModule();
    nativeShadowFilterNodeModule();

    const auto base = nativeShadowBaseNodeConstructor();
    JSValue bindings = JSValue::object(nullptr);
    const auto ctor = makeClass(
        "ShadowNode", 2, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto light = arg(args, 0, nullptr);
            auto self = superConstruct(target, captures, {light});
            auto shadow = arg(args, 1, JSValue{});
            if (!shadow.toBoolean()) shadow = light.isObject() ? light.get("shadow") : JSValue{};
            self.set("shadow", shadow);
            self.set("shadowMap", JSValue(nullptr));
            self.set("vsmShadowMapVertical", JSValue(nullptr));
            self.set("vsmShadowMapHorizontal", JSValue(nullptr));
            self.set("vsmMaterialVertical", JSValue(nullptr));
            self.set("vsmMaterialHorizontal", JSValue(nullptr));
            self.set("_node", JSValue(nullptr));
            self.set("_currentShadowType", JSValue(nullptr));
            self.set("_cameraFrameId", createNativeWeakMap());
            self.set("isShadowNode", true);
            self.set("depthLayer", JSValue(0));
            return self;
        },
        {}, "", "");

    const auto prototype = ctor.get("prototype");
    method(prototype, "getShadowFilterFn", 1, getShadowFilterFn, {}, "");
    method(prototype, "dispose", 0, disposeMethod, {}, "");
    method(prototype, "_reset", 0, resetMethod, {}, "");
    method(prototype, "setupShadowFilter", 2, setupShadowFilter, {}, "");
    method(prototype, "setupShadowCoord", 2, setupShadowCoord, {}, "");
    method(prototype, "setupRenderTarget", 2, setupRenderTarget, {}, "");
    method(prototype, "setupShadow", 1, setupShadow, {}, "");
    method(prototype, "setup", 1, setup, {}, "");
    method(prototype, "renderShadow", 1, renderShadow, {}, "");
    method(prototype, "updateShadow", 1, updateShadow, {}, "");
    method(prototype, "vsmPass", 1, vsmPass, {}, "");
    method(prototype, "updateBefore", 1, updateBefore, {}, "");
    own(bindings, "shadow", JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto ctor = captures.at(0);
            auto shadowArgument = arg(args, 1, JSValue{});
            return ctor.construct({arg(args, 0, nullptr), shadowArgument});
        },
        {ctor}));
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeShadowNodeBinding(const std::string& name) {
    const auto keeper = nativeShadowNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeShadowNodeConstructor() { return nativeShadowNodeBinding("default"); }