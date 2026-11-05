#include "nodes/utils/ReflectorNode.h"

#include "constants.h"
#include "core/JSGlobal.h"
#include "core/JSMap.h"
#include "core/Object3D.h"
#include "core/JSWeakMap.h"
#include "core/RenderTarget.h"
#include "math/Matrix4.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/display/ScreenNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "textures/DepthTexture.h"

#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& defaults() { static thread_local JSWeakValue weak; return weak; }
JSWeakValue& reflectorState() { static thread_local JSWeakValue weak; return weak; }

[[noreturn]] JSValue notPorted(const char* name) {
    throw std::runtime_error(std::string("ReflectorNode: ") + name + " is not ported yet");
}

// const _defaultRT = new RenderTarget(); _defaultRT.depthTexture = new DepthTexture( 1, 1 );
JSValue defaultRenderTarget() {
    if (const auto cached = defaults().lock(); cached && cached->toBoolean()) {
        return cached->get("renderTarget");
    }
    auto state = JSValue::object(nullptr);
    auto renderTarget = nativeRenderTargetConstructor().construct();
    renderTarget.set("depthTexture", nativeDepthTextureConstructor().construct({JSValue(1), JSValue(1)}));
    state.set("renderTarget", renderTarget);
    defaults() = JSWeakValue(state);
    return renderTarget;
}

// const _defaultUV = screenUV.flipX();
JSValue defaultUV() {
    if (const auto cached = defaults().lock(); cached && cached->toBoolean()) {
        return cached->get("uv");
    }
    const auto uv = invoke(nativeScreenNodeBinding("screenUV"), "flipX")({});
    auto state = defaults().lock() ? *defaults().lock() : JSValue::object(nullptr);
    state.set("uv", uv);
    defaults() = JSWeakValue(state);
    return uv;
}

// ReflectorBaseNode constructor( textureNode, parameters = {} )
JSValue reflectorBaseConstructor(const JSValue& base) {
    return makeClass(
        "ReflectorBaseNode", 2, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {});
            const auto textureNode = arg(args, 0, nullptr);
            const auto parameters = arg(args, 1, JSValue::object(nullptr));
            const auto option = [&parameters](const char* name, const JSValue& fallback) {
                const auto value = parameters.isObject() ? parameters.get(name) : JSValue{};
                return value.isUndefined() ? fallback : value;
            };
            self.set("textureNode", textureNode);
            self.set("target", option("target", nativeObject3DConstructor().construct()));
            self.set("resolutionScale", option("resolutionScale", JSValue(1)));
            if (parameters.isObject() && !parameters.get("resolution").isUndefined()) {
                self.set("resolutionScale", parameters.get("resolution"));
            }
            self.set("generateMipmaps", option("generateMipmaps", JSValue(false)));
            self.set("bounces", option("bounces", JSValue(true)));
            self.set("depth", option("depth", JSValue(false)));
            self.set("samples", option("samples", JSValue(0)));
            self.set("updateBeforeType",
                     option("bounces", JSValue(true)).toBoolean()
                         ? nativeNodeConstantsBinding("NodeUpdateType").get("RENDER")
                         : nativeNodeConstantsBinding("NodeUpdateType").get("FRAME"));
            self.set("virtualCameras", createNativeWeakMap());
            self.set("renderTargets", createNativeMap());
            self.set("forceUpdate", false);
            self.set("hasOutput", false);
            return self;
        },
        {}, "", "");
}

JSValue baseIsCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// _updateResolution( renderTarget, renderer ) { renderer.getDrawingBufferSize( _size ); renderTarget.setSize(...) }
JSValue baseUpdateResolution(const JSValue& self, const Args& args, const Args&) {
    const auto renderTarget = arg(args, 0);
    const auto renderer = arg(args, 1);
    const auto resolution = self.get("resolutionScale").toNumber();
    auto size = nativeVector2Constructor().construct();
    size = renderer.get("getDrawingBufferSize").call(renderer, {size});
    renderTarget.get("setSize").call(
        renderTarget, {JSValue(std::round(size.get("width").toNumber() * resolution)),
                       JSValue(std::round(size.get("height").toNumber() * resolution))});
    return JSValue{};
}

// setup( builder ) { this._updateResolution( _defaultRT, builder.renderer ); return super.setup( builder ); }
JSValue baseSetup(const JSValue& self, const Args& args, const Args& captures) {
    const auto builder = arg(args, 0);
    defaultRenderTarget();
    self.get("_updateResolution").call(self, {defaultRenderTarget(), builder.get("renderer")});
    const auto basePrototype = captures.at(0);
    const auto baseSetup = basePrototype.get("setup");
    if (baseSetup.toBoolean()) return baseSetup.call(self, {builder});
    return self;
}

// dispose() { super.dispose(); for ( const renderTarget of this.renderTargets.values() ) renderTarget.dispose(); }
JSValue baseDispose(const JSValue& rawSelf, const Args&, const Args& captures) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto basePrototype = captures.at(0);
    const auto baseDispose = basePrototype.get("dispose");
    if (baseDispose.toBoolean()) baseDispose.call(self, {});
    const auto renderTargets = self.get("renderTargets");
    renderTargets.get("forEach").call(renderTargets, {JSValue::function(
        +[](const JSValue&, const Args& forEachArgs, const Args&) -> JSValue {
            const auto renderTarget = arg(forEachArgs, 0);
            renderTarget.get("dispose").call(renderTarget, {});
            return JSValue{};
        })});
    return JSValue{};
}

// getVirtualCamera( camera ) { ... camera.clone() cached per camera ... }
JSValue baseGetVirtualCamera(const JSValue& self, const Args& args, const Args&) {
    const auto camera = arg(args, 0);
    const auto cameras = self.get("virtualCameras");
    auto virtualCamera = cameras.get("get").call(cameras, {camera});
    if (!virtualCamera.toBoolean()) {
        virtualCamera = camera.get("clone").call(camera, {});
        cameras.get("set").call(cameras, {camera, virtualCamera});
    }
    return virtualCamera;
}

// getRenderTarget( camera ) { ... new RenderTarget( 1, 1, { type: HalfFloatType, samples } ) ... }
JSValue baseGetRenderTarget(const JSValue& self, const Args& args, const Args&) {
    const auto camera = arg(args, 0);
    const auto targets = self.get("renderTargets");
    auto renderTarget = targets.get("get").call(targets, {camera});
    if (!renderTarget.toBoolean()) {
        auto options = JSValue::object(nullptr);
        options.set("type", JSValue(static_cast<double>(HalfFloatType)));
        options.set("samples", self.get("samples"));
        renderTarget = nativeRenderTargetConstructor().construct({JSValue(1), JSValue(1), options});
        if (self.get("generateMipmaps").strictEquals(true)) {
            renderTarget.get("texture").set(
                "minFilter", JSValue(static_cast<double>(LinearMipMapLinearFilter)));
            renderTarget.get("texture").set("generateMipmaps", true);
        }
        targets.get("set").call(targets, {camera, renderTarget});
    }
    return renderTarget;
}

JSValue baseUpdateBefore(const JSValue&, const Args&, const Args&) {
    return notPorted("ReflectorBaseNode.updateBefore");
}

JSValue reflectorGetDepthNode(const JSValue&, const Args&, const Args&) {
    return notPorted("ReflectorNode.getDepthNode");
}
JSValue reflectorClone(const JSValue&, const Args&, const Args&) {
    return notPorted("ReflectorNode.clone");
}
JSValue reflectorDispose(const JSValue&, const Args&, const Args&) {
    return notPorted("ReflectorNode.dispose");
}
}  // namespace

JSValue nativeReflectorNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ReflectorNode module");
    nativeTextureNodeModule();

    const auto nodeBase = nativeNodeConstructor();
    const auto baseConstructor = reflectorBaseConstructor(nodeBase);
    {
        const auto prototype = baseConstructor.get("prototype");
        method(prototype, "isCacheable", 0, baseIsCacheable, {}, "");
        method(prototype, "_updateResolution", 2, baseUpdateResolution, {}, "");
        method(prototype, "setup", 1, baseSetup, {nodeBase.get("prototype")}, "");
        method(prototype, "dispose", 0, baseDispose, {nodeBase.get("prototype")}, "");
        method(prototype, "getVirtualCamera", 1, baseGetVirtualCamera, {}, "");
        method(prototype, "getRenderTarget", 1, baseGetRenderTarget, {}, "");
        method(prototype, "updateBefore", 1, baseUpdateBefore, {}, "");
    }

    JSValue bindings = JSValue::object(nullptr);
    const auto textureConstructor = nativeTextureNodeConstructor();
    const auto reflectorConstructor = makeClass(
        "ReflectorNode", 1, textureConstructor,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto parameters = arg(args, 0, JSValue::object(nullptr));
            const auto providedTexture = parameters.isObject() ? parameters.get("defaultTexture") : JSValue{};
            const auto texture = providedTexture.toBoolean() ? providedTexture
                                                             : defaultRenderTarget().get("texture");
            auto self = superConstruct(target, captures, {texture, defaultUV()});
            const auto providedReflector = parameters.isObject() ? parameters.get("reflector") : JSValue{};
            self.set("_reflectorBaseNode",
                     providedReflector.toBoolean()
                         ? providedReflector
                         : nativeReflectorBaseNodeConstructor().construct({self, parameters}));
            self.set("_depthNode", JSValue(nullptr));
            self.get("setUpdateMatrix").call(self, {JSValue(false)});
            return self;
        },
        {}, "", "");
    {
        const auto prototype = reflectorConstructor.get("prototype");
        // getters for reflector and target read the base node, like the pinned accessors.
        const auto defineProperty = nativeGlobalBinding("Object").get("defineProperty");
        auto reflectorGetter = JSValue::function(
            +[](const JSValue& self, const Args&, const Args&) -> JSValue {
                return self.get("_reflectorBaseNode");
            });
        auto targetGetter = JSValue::function(
            +[](const JSValue& self, const Args&, const Args&) -> JSValue {
                return self.get("_reflectorBaseNode").get("target");
            });
        for (const auto& [name, getter] : std::initializer_list<std::pair<const char*, JSValue>>{
                 {"reflector", reflectorGetter}, {"target", targetGetter}}) {
            auto descriptor = JSValue::object(nullptr);
            descriptor.set("get", getter);
            descriptor.set("enumerable", false);
            descriptor.set("configurable", false);
            defineProperty.call({}, {prototype, JSValue(name), descriptor});
        }
        method(prototype, "getDepthNode", 0, reflectorGetDepthNode, {}, "");
        method(prototype, "clone", 0, reflectorClone, {}, "");
        method(prototype, "dispose", 0, reflectorDispose, {}, "");
    }
    own(bindings, "default", reflectorConstructor);
    own(bindings, "ReflectorBaseNode", baseConstructor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeReflectorNodeBinding(const std::string& name) {
    const auto keeper = nativeReflectorNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeReflectorNodeConstructor() { return nativeReflectorNodeBinding("default"); }
JSValue nativeReflectorBaseNodeConstructor() { return nativeReflectorNodeBinding("ReflectorBaseNode"); }