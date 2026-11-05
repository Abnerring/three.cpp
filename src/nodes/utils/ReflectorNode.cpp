#include "nodes/utils/ReflectorNode.h"

#include "constants.h"
#include "core/JSGlobal.h"
#include "core/JSMap.h"
#include "core/JSPromise.h"
#include "core/Object3D.h"
#include "core/JSWeakMap.h"
#include "core/RenderTarget.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
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

// The frame hook builds the reflection plane, mirrors the virtual camera and renders the scene from it.
// Module level temporaries mirror the pinned scratch objects.
JSWeakValue& scratch() { static thread_local JSWeakValue weak; return weak; }
bool& inReflector() { static thread_local bool value = false; return value; }

JSValue scratchObjects() {
    if (const auto cached = scratch().lock(); cached && cached->toBoolean()) return *cached;
    auto objects = JSValue::object(nullptr);
    objects.set("plane", nativePlaneConstructor().construct());
    objects.set("normal", nativeVector3Constructor().construct());
    objects.set("reflectorWorldPosition", nativeVector3Constructor().construct());
    objects.set("cameraWorldPosition", nativeVector3Constructor().construct());
    objects.set("rotationMatrix", nativeMatrix4Constructor().construct());
    objects.set("lookAtPosition", nativeVector3Constructor().construct({JSValue(0), JSValue(0), JSValue(-1)}));
    objects.set("clipPlane", nativeVector4Constructor().construct());
    objects.set("view", nativeVector3Constructor().construct());
    objects.set("target", nativeVector3Constructor().construct());
    objects.set("q", nativeVector4Constructor().construct());
    objects.set("size", nativeVector2Constructor().construct());
    scratch() = JSWeakValue(objects);
    return objects;
}

JSValue baseUpdateBefore(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    if (!self.get("bounces").toBoolean() && inReflector()) return JSValue(false);

    inReflector() = true;
    const auto frame = arg(args, 0);
    auto scene = frame.get("scene");
    const auto camera = frame.get("camera");
    auto renderer = frame.get("renderer");
    auto material = frame.get("material");
    const auto target = self.get("target");
    const auto scratch = scratchObjects();

    auto virtualCamera = self.get("getVirtualCamera").call(self, {camera});
    const auto renderTarget = self.get("getRenderTarget").call(self, {virtualCamera});
    self.get("_updateResolution").call(self, {renderTarget, renderer});

    auto reflectorWorldPosition = scratch.get("reflectorWorldPosition");
    auto cameraWorldPosition = scratch.get("cameraWorldPosition");
    auto rotationMatrix = scratch.get("rotationMatrix");
    auto normal = scratch.get("normal");
    auto lookAtPosition = scratch.get("lookAtPosition");
    auto view = scratch.get("view");
    auto reflectTarget = scratch.get("target");

    reflectorWorldPosition = reflectorWorldPosition.get("setFromMatrixPosition").call(
        reflectorWorldPosition, {target.get("matrixWorld")});
    cameraWorldPosition = cameraWorldPosition.get("setFromMatrixPosition").call(
        cameraWorldPosition, {camera.get("matrixWorld")});
    rotationMatrix = rotationMatrix.get("extractRotation").call(
        rotationMatrix, {target.get("matrixWorld")});
    normal = normal.get("set").call(normal, {JSValue(0), JSValue(0), JSValue(1)});
    normal = normal.get("applyMatrix4").call(normal, {rotationMatrix});
    view = view.get("subVectors").call(view, {reflectorWorldPosition, cameraWorldPosition});

    const auto facingAway = view.get("dot").call(view, {normal}).toNumber() > 0;
    bool needsClear = false;
    if (facingAway && !self.get("forceUpdate").toBoolean()) {
        if (!self.get("hasOutput").toBoolean()) {
            inReflector() = false;
            return JSValue{};
        }
        needsClear = true;
    }

    view = view.get("reflect").call(view, {normal});
    view = view.get("negate").call(view, {});
    view = view.get("add").call(view, {reflectorWorldPosition});

    rotationMatrix = rotationMatrix.get("extractRotation").call(rotationMatrix, {camera.get("matrixWorld")});
    lookAtPosition = lookAtPosition.get("set").call(
        lookAtPosition, {JSValue(0), JSValue(0), JSValue(-1)});
    lookAtPosition = lookAtPosition.get("applyMatrix4").call(lookAtPosition, {rotationMatrix});
    lookAtPosition = lookAtPosition.get("add").call(lookAtPosition, {cameraWorldPosition});

    reflectTarget = reflectTarget.get("subVectors").call(reflectTarget, {reflectorWorldPosition, lookAtPosition});
    reflectTarget = reflectTarget.get("reflect").call(reflectTarget, {normal});
    reflectTarget = reflectTarget.get("negate").call(reflectTarget, {});
    reflectTarget = reflectTarget.get("add").call(reflectTarget, {reflectorWorldPosition});

    virtualCamera.set("coordinateSystem", camera.get("coordinateSystem"));
    auto virtualPosition = virtualCamera.get("position");
    virtualPosition = virtualPosition.get("copy").call(virtualPosition, {view});
    virtualCamera.set("position", virtualPosition);
    auto virtualUp = virtualCamera.get("up");
    virtualUp = virtualUp.get("set").call(virtualUp, {JSValue(0), JSValue(1), JSValue(0)});
    virtualUp = virtualUp.get("applyMatrix4").call(virtualUp, {rotationMatrix});
    virtualUp = virtualUp.get("reflect").call(virtualUp, {normal});
    virtualCamera.set("up", virtualUp);
    virtualCamera.get("lookAt").call(virtualCamera, {reflectTarget});
    virtualCamera.set("near", camera.get("near"));
    virtualCamera.set("far", camera.get("far"));
    virtualCamera.get("updateMatrixWorld").call(virtualCamera, {});
    auto virtualProjection = virtualCamera.get("projectionMatrix");
    virtualProjection = virtualProjection.get("copy").call(virtualProjection, {camera.get("projectionMatrix")});
    virtualCamera.set("projectionMatrix", virtualProjection);

    // The mirrored projection flips the projection matrix entries for the reflection.
    auto projectionElements = virtualProjection.get("elements");
    projectionElements.set(JSValue(8.0), JSValue(-projectionElements.get(JSValue(8.0)).toNumber()));
    projectionElements.set(JSValue(12.0), JSValue(-projectionElements.get(JSValue(12.0)).toNumber()));
    virtualProjection.set("elements", projectionElements);
    virtualCamera.set("projectionMatrix", virtualProjection);

    auto reflectorPlane = scratch.get("plane");
    reflectorPlane = reflectorPlane.get("setFromNormalAndCoplanarPoint").call(
        reflectorPlane, {normal, reflectorWorldPosition});
    reflectorPlane = reflectorPlane.get("applyMatrix4").call(
        reflectorPlane, {virtualCamera.get("matrixWorldInverse")});
    auto clipPlane = scratch.get("clipPlane");
    clipPlane = clipPlane.get("set").call(
        clipPlane, {reflectorPlane.get("normal").get("x"), reflectorPlane.get("normal").get("y"),
                    reflectorPlane.get("normal").get("z"), reflectorPlane.get("constant")});

    auto projection = virtualCamera.get("projectionMatrix");
    auto elements = projection.get("elements");
    auto q = scratch.get("q");
    const auto signX = (clipPlane.get("x").toNumber() >= 0 ? 1.0 : -1.0);
    const auto signY = (clipPlane.get("y").toNumber() >= 0 ? 1.0 : -1.0);
    q = q.get("set").call(
        q, {JSValue((signX + elements.get(JSValue(8.0)).toNumber()) / elements.get(JSValue(0.0)).toNumber()),
            JSValue((signY + elements.get(JSValue(9.0)).toNumber()) / elements.get(JSValue(5.0)).toNumber()),
            JSValue(-1.0),
            JSValue((1.0 + elements.get(JSValue(10.0)).toNumber()) / elements.get(JSValue(14.0)).toNumber())});
    const auto qDot = clipPlane.get("dot").call(clipPlane, {q}).toNumber();
    clipPlane = clipPlane.get("multiplyScalar").call(clipPlane, {JSValue(1.0 / qDot)});

    const double clipBias = 0;
    elements.set(JSValue(2.0), clipPlane.get("x"));
    elements.set(JSValue(6.0), clipPlane.get("y"));
    const bool isWebGPU = renderer.get("coordinateSystem").toNumber() ==
                          static_cast<double>(WebGPUCoordinateSystem);
    const auto clipZ = clipPlane.get("z").toNumber();
    elements.set(JSValue(10.0), JSValue(isWebGPU ? (clipZ - clipBias) : clipZ));
    elements.set(JSValue(14.0), clipPlane.get("w"));
    projection.set("elements", elements);
    virtualCamera.set("projectionMatrix", projection);

    self.get("textureNode").set("value", renderTarget.get("texture"));
    if (self.get("depth").strictEquals(true)) {
        auto textureNode = self.get("textureNode");
        auto depthNode = textureNode.get("getDepthNode").call(textureNode, {});
        depthNode.set("value", renderTarget.get("depthTexture"));
    }

    material.set("visible", false);
    const auto currentRenderTarget = renderer.get("getRenderTarget").call(renderer, {});
    const auto currentMRT = renderer.get("getMRT").call(renderer, {});
    const auto currentAutoClear = renderer.get("autoClear");
    renderer.get("setMRT").call(renderer, {JSValue(nullptr)});
    renderer.get("setRenderTarget").call(renderer, {renderTarget});
    renderer.set("autoClear", true);

    const auto previousName = scene.get("name");
    const auto baseName = previousName.toBoolean() ? previousName.toString() : std::string("Scene");
    scene.set("name", JSValue(baseName + " [ Reflector ]"));
    if (needsClear) {
        renderer.get("clear").call(renderer, {});
        self.set("hasOutput", false);
    } else {
        renderer.get("render").call(renderer, {scene, virtualCamera});
        self.set("hasOutput", true);
    }
    scene.set("name", previousName);

    renderer.get("setMRT").call(renderer, {currentMRT});
    renderer.get("setRenderTarget").call(renderer, {currentRenderTarget});
    renderer.set("autoClear", currentAutoClear);
    material.set("visible", true);
    inReflector() = false;
    self.set("forceUpdate", false);
    return JSValue{};
}

// getDepthNode() { if ( this._depthNode === null ) { ... throw when the reflector has no depth ... new ReflectorNode(...) } }
JSValue reflectorGetDepthNode(const JSValue& rawSelf, const Args&, const Args& captures) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    auto depthNode = self.get("_depthNode");
    if (!depthNode.toBoolean()) {
        const auto baseNode = self.get("_reflectorBaseNode");
        if (!baseNode.get("depth").strictEquals(true)) {
            throw JSThrownValue(nativeGlobalBinding("Error").construct(
                {"THREE.ReflectorNode: Depth node can only be requested when the reflector is created with { depth: true }. "}));
        }
        auto parameters = JSValue::object(nullptr);
        parameters.set("defaultTexture", defaultRenderTarget().get("depthTexture"));
        parameters.set("reflector", baseNode);
        depthNode = captures.at(0).construct({parameters});
        self.set("_depthNode", depthNode);
    }
    return depthNode;
}

// clone() { const newNode = new this.constructor( this.reflectorNode ); ... copy sampling nodes ...
//           newNode._reflectorBaseNode = this._reflectorBaseNode; }
JSValue reflectorClone(const JSValue& self, const Args&, const Args& captures) {
    auto clone = captures.at(0).construct({self.get("reflectorNode")});
    for (const char* field : {"uvNode", "levelNode", "biasNode", "sampler", "depthNode",
                              "compareNode", "gradNode", "gatherNode", "offsetNode"}) {
        clone.set(field, self.get(field));
    }
    clone.set("_reflectorBaseNode", self.get("_reflectorBaseNode"));
    return clone;
}

// dispose() { super.dispose(); this._reflectorBaseNode.dispose(); }
JSValue reflectorDispose(const JSValue& rawSelf, const Args&, const Args& captures) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto basePrototype = captures.at(0);
    const auto baseDispose = basePrototype.get("dispose");
    if (baseDispose.toBoolean()) baseDispose.call(self, {});
    self.get("_reflectorBaseNode").get("dispose").call(self.get("_reflectorBaseNode"), {});
    return JSValue{};
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
        method(prototype, "getDepthNode", 0, reflectorGetDepthNode,
               {reflectorConstructor}, "");
        method(prototype, "clone", 0, reflectorClone, {reflectorConstructor}, "");
        method(prototype, "dispose", 0, reflectorDispose, {nativeTextureNodeConstructor().get("prototype")}, "");
    }
    // ReflectorBaseNode accessors: the deprecated resolution alias forwards to resolutionScale.
    {
        const auto defineProperty = nativeGlobalBinding("Object").get("defineProperty");
        auto descriptor = JSValue::object(nullptr);
        descriptor.set("get", JSValue::function(
            +[](const JSValue& self, const Args&, const Args&) -> JSValue {
                return self.get("resolutionScale");
            }));
        descriptor.set("set", JSValue::function(
            +[](const JSValue& rawSelf, const Args& args, const Args&) -> JSValue {
                JSValue& self = const_cast<JSValue&>(rawSelf);
                self.set("resolutionScale", arg(args, 0, JSValue(1)));
                return JSValue{};
            }));
        descriptor.set("enumerable", false);
        descriptor.set("configurable", false);
        defineProperty.call({}, {baseConstructor.get("prototype"), JSValue("resolution"), descriptor});
    }
    own(bindings, "reflector", JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            return captures.at(0).construct({arg(args, 0, JSValue::object(nullptr))});
        },
        {reflectorConstructor}));
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