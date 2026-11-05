#include "nodes/utils/RTTNode.h"

#include "constants.h"
#include "core/RenderTarget.h"
#include "math/Vector2.h"
#include "materials/nodes/NodeMaterial.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/accessors/UV.h"
#include "nodes/core/ContextNode.h"
#include "nodes/core/NodeConstants.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/utils/UtilNodeSources.inc"
#include "renderers/common/QuadMesh.h"
#include "renderers/common/RendererUtils.h"
#include "utils.h"

#include <cmath>
#include <string>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _size = new Vector2();  shared by the render update callback.
JSValue& sizeKeeper() { static thread_local JSValue value; return value; }

JSValue sizeVector() {
    if (!sizeKeeper().toBoolean()) sizeKeeper() = nativeVector2Constructor().construct();
    return sizeKeeper();
}

// get autoResize() { return this.width === null; }
JSValue autoResizeGetter(const JSValue& self, const Args&, const Args&) {
    return JSValue(self.get("width").isNull());
}

// get value() { return this.renderTarget ? this.renderTarget.texture : null; }
JSValue valueGetter(const JSValue& self, const Args&, const Args&) {
    const auto renderTarget = self.get("renderTarget");
    return renderTarget.toBoolean() ? renderTarget.get("texture") : JSValue(nullptr);
}

// set value( value ) { if ( this.renderTarget && value !== this.renderTarget.texture ) error( '...' ); }
JSValue valueSetter(const JSValue& self, const Args& args, const Args&) {
    const auto value = arg(args, 0, nullptr);
    const auto renderTarget = self.get("renderTarget");
    if (renderTarget.toBoolean() && !value.sameValue(renderTarget.get("texture"))) {
        ThreeUtils::error({JSValue("TSL: \"rtt()\" does not allow overwriting the value.")});
    }
    return {};
}

// setup( builder ) { ... }
JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto quadMesh = self.get("_quadMesh");
    const auto material = quadMesh.get("material");
    auto materialRef = material;
    materialRef.set("contextNode", nativeContextNodeBinding("context").call({}, {invoke(builder, "getSharedContext")({})}));
    materialRef.set("fragmentNode", self.get("node"));
    materialRef.set("name", JSValue("RTT"));
    materialRef.set("needsUpdate", true);
    const auto baseSetup = nativeTextureNodeConstructor().get("prototype").get("setup");
    if (baseSetup.isCallable()) return baseSetup.call(self, {builder});
    return {};
}

// setSize( width, height ) { ... }
JSValue setSize(const JSValue& self, const Args& args, const Args&) {
    const auto width = arg(args, 0, nullptr), height = arg(args, 1, nullptr);
    const auto scale = self.get("_resolutionScale").toNumber();
    const auto effectiveWidth = JSValue(std::floor(width.toNumber() * scale));
    const auto effectiveHeight = JSValue(std::floor(height.toNumber() * scale));
    invoke(self.get("renderTarget"), "setSize")({effectiveWidth, effectiveHeight});
    auto node = self;
    node.set("textureNeedsUpdate", true);
    return {};
}

// setResolutionScale( resolutionScale ) { ... }
JSValue setResolutionScale(const JSValue& self, const Args& args, const Args&) {
    auto node = self;
    node.set("_resolutionScale", arg(args, 0, nullptr));
    if (autoResizeGetter(self, {}, {}).strictEquals(false)) {
        invoke(self, "setSize")({self.get("width"), self.get("height")});
    }
    return self;
}

// getResolutionScale() { return this._resolutionScale; }
JSValue getResolutionScale(const JSValue& self, const Args&, const Args&) {
    return self.get("_resolutionScale");
}

// updateBefore( frame ) { ... }
JSValue updateBefore(const JSValue& self, const Args& args, const Args&) {
    const auto frame = arg(args, 0, nullptr);
    const auto renderer = frame.get("renderer");
    auto node = self;
    if (self.get("textureNeedsUpdate").strictEquals(false) && self.get("autoUpdate").strictEquals(false)) {
        return {};
    }
    node.set("textureNeedsUpdate", false);
    const auto resolutionScale = self.get("_resolutionScale").toNumber();
    if (autoResizeGetter(self, {}, {}).strictEquals(true)) {
        const auto size = invoke(renderer, "getDrawingBufferSize")({sizeVector()});
        const auto effectiveWidth = JSValue(std::floor(size.get("width").toNumber() * resolutionScale));
        const auto effectiveHeight = JSValue(std::floor(size.get("height").toNumber() * resolutionScale));
        const auto renderTarget = self.get("renderTarget");
        if (effectiveWidth.toNumber() != renderTarget.get("width").toNumber() ||
            effectiveHeight.toNumber() != renderTarget.get("height").toNumber()) {
            invoke(renderTarget, "setSize")({effectiveWidth, effectiveHeight});
            node.set("textureNeedsUpdate", true);
        }
    }
    const auto callName = self.get("name").toBoolean() ? self.get("name") : self.get("node").get("name");
    auto name = JSValue("RTT");
    if (callName.toBoolean()) name = JSValue(callName.toString() + " [ RTT ]");
    auto quadMesh = self.get("_quadMesh");
    quadMesh.set("name", name);
    const auto state = self.get("_rendererState");
    nativeRendererUtilsBinding("resetRendererState").call({}, {renderer, state});
    invoke(renderer, "setClearColor")({state.get("clearColor"), state.get("clearAlpha")});
    invoke(renderer, "setRenderTarget")({self.get("renderTarget")});
    invoke(quadMesh, "render")({renderer});
    nativeRendererUtilsBinding("restoreRendererState").call({}, {renderer, state});
    return {};
}

// clone() { const newNode = new TextureNode( this.value, this.uvNode, this.levelNode ); ... }
JSValue clone(const JSValue& self, const Args&, const Args&) {
    auto node = self;
    const auto newNode = nativeTextureNodeConstructor().construct(
        {valueGetter(self, {}, {}), self.get("uvNode"), self.get("levelNode")});
    auto created = newNode;
    created.set("sampler", self.get("sampler"));
    created.set("referenceNode", self);
    return newNode;
}

// dispose() { this.renderTarget.dispose(); this._quadMesh.material.dispose(); super.dispose(); }
JSValue dispose(const JSValue& self, const Args&, const Args&) {
    invoke(self.get("renderTarget"), "dispose")({});
    invoke(self.get("_quadMesh").get("material"), "dispose")({});
    const auto baseDispose = nativeTextureNodeConstructor().get("prototype").get("dispose");
    if (baseDispose.isCallable()) baseDispose.call(self, {});
    return {};
}
}  // namespace

JSValue nativeRTTNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "RTTNode module");
    nativeTSLCore();
    nativeTextureNodeModule();
    nativeUVModule();
    nativeContextNodeModule();
    nativeNodeMaterialModule();
    nativeQuadMeshModule();
    nativeRendererUtilsModule();

    auto bindings = JSValue::object(nullptr);
    const auto base = nativeTextureNodeConstructor();
    auto ctor = makeClass(
        "RTTNode", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto node = arg(args, 0, nullptr);
            const auto width = arg(args, 1, nullptr);
            const auto height = arg(args, 2, nullptr);
            const auto options = arg(args, 3, JSValue::object(nullptr));
            auto autoUpdate = options.get("autoUpdate");
            if (autoUpdate.isUndefined()) autoUpdate = JSValue(true);
            auto resolutionScale = options.get("resolutionScale");
            if (resolutionScale.isUndefined()) resolutionScale = JSValue(1.0);

            // const renderTarget = new RenderTarget( width ?? 1, height ?? 1, { type: HalfFloatType, ...options } )
            auto renderTargetOptions = JSValue::object(nullptr);
            renderTargetOptions.set("type", JSValue(static_cast<double>(HalfFloatType)));
            const auto optionNames = options.ownPropertyNames();
            for (const auto& name : optionNames) {
                const auto descriptor = options.getOwnPropertyDescriptor(name);
                if (!descriptor || !descriptor->enumerable.value_or(false)) continue;
                renderTargetOptions.set(name.c_str(), options.get(name.c_str()));
            }
            const auto renderTarget = nativeRenderTargetConstructor().construct(
                {width.isNull() ? JSValue(1.0) : width, height.isNull() ? JSValue(1.0) : height,
                 renderTargetOptions});

            // super( renderTarget.texture, uv() )
            auto self = superConstruct(
                target, captures,
                {renderTarget.get("texture"), nativeUVBinding("uv").call({}, {})});
            self.set("isRTTNode", true);
            self.set("node", node);
            self.set("width", width.isUndefined() ? JSValue(nullptr) : width);
            self.set("height", height.isUndefined() ? JSValue(nullptr) : height);
            self.set("renderTarget", renderTarget);
            self.set("textureNeedsUpdate", true);
            self.set("autoUpdate", autoUpdate);
            self.set("_resolutionScale", resolutionScale);
            self.set("_quadMesh", nativeQuadMeshConstructor().construct({nativeNodeMaterialConstructor().construct()}));
            self.set("_rendererState", JSValue::object(nullptr));
            self.set("updateBeforeType", nativeNodeConstantsBinding("NodeUpdateType").get("FRAME"));
            return self;
        },
        {}, RTTNodeSource::classSource, RTTNodeSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "setup", 1, setup, {}, RTTNodeSource::setupSource);
    method(prototype, "setSize", 2, setSize, {}, RTTNodeSource::setSizeSource);
    method(prototype, "setResolutionScale", 1, setResolutionScale, {},
           RTTNodeSource::setResolutionScaleSource);
    method(prototype, "getResolutionScale", 0, getResolutionScale, {},
           RTTNodeSource::getResolutionScaleSource);
    method(prototype, "updateBefore", 1, updateBefore, {}, RTTNodeSource::updateBeforeSource);
    method(prototype, "clone", 0, clone, {}, RTTNodeSource::cloneSource);
    method(prototype, "dispose", 0, dispose, {}, RTTNodeSource::disposeSource);

    // get autoResize() / get value() / set value( value )
    JSPropertyDescriptor autoResize;
    autoResize.configurable = true;
    autoResize.getter = JSValue::function(autoResizeGetter, {}, "get autoResize", 0);
    autoResize.getter->setFunctionSourceText(RTTNodeSource::autoResizeSource);
    prototype.defineProperty("autoResize", autoResize);

    JSPropertyDescriptor value;
    value.configurable = true;
    value.getter = JSValue::function(valueGetter, {}, "get value", 0);
    value.getter->setFunctionSourceText(RTTNodeSource::valueSource);
    value.setter = JSValue::function(valueSetter, {}, "set value", 1);
    prototype.defineProperty("value", value);

    // export const rtt = ( node, ...params ) => new RTTNode( nodeObject( node ), ...params );
    const auto rtt = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            Args ctorArgs{invoke(captures[0], "call")({JSValue::object(nullptr), arg(args, 0, nullptr)})};
            for (std::size_t i = 1; i < args.size(); ++i) ctorArgs.push_back(args[i]);
            return captures[1].construct(ctorArgs);
        },
        {nativeTSLCoreBinding("nodeObject"), ctor}, "rtt", 1);

    // export const convertToTexture = ( node, ...params ) => { ... }
    const auto convertToTexture = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto node = arg(args, 0, nullptr);
            if (node.get("isSampleNode").strictEquals(true) ||
                node.get("isTextureNode").strictEquals(true)) {
                return node;
            }
            if (node.get("isPassNode").strictEquals(true)) {
                return invoke(node, "getTextureNode")({});
            }
            Args callArgs{node};
            for (std::size_t i = 1; i < args.size(); ++i) callArgs.push_back(args[i]);
            return captures[0].call({}, callArgs);
        },
        {rtt}, "convertToTexture", 1);

    own(bindings, "default", ctor);
    own(bindings, "rtt", rtt);
    own(bindings, "convertToTexture", convertToTexture);
    return publish(bindings, lexical(), weak);
}

JSValue nativeRTTNodeBinding(const std::string& name) {
    const auto keeper = nativeRTTNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeRTTNodeConstructor() {
    return nativeRTTNodeBinding("default");
}
