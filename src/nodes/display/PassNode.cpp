#include "nodes/display/PassNode.h"

#include "core/JSWeakMap.h"
#include "nodes/accessors/TextureNode.h"
#include "constants.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "nodes/core/Node.h"
#include "nodes/core/UniformNode.h"
#include "nodes/display/ViewportDepthNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "core/RenderTarget.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include "textures/DepthTexture.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace {
using namespace LightingNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// PassTextureNode = class extends TextureNode { constructor( passNode, texture ) { super( texture ); ...
JSValue passTextureNodeConstructor(const JSValue& basePrototype, const JSValue& baseConstructor) {
    return makeClass(
        "PassTextureNode", 2, baseConstructor,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 1, nullptr)});
            self.set("passNode", arg(args, 0, nullptr));
            self.set("isPassTextureNode", true);
            auto setUpdateMatrix = self.get("setUpdateMatrix");
            if (setUpdateMatrix.toBoolean()) setUpdateMatrix.call(self, {JSValue(false)});
            return self;
        },
        {}, "", "");
}

// constructor( passNode, textureName, previousTexture = false ) { super( passNode, null ); ... }
JSValue passMultipleTextureNodeConstructor(const JSValue& baseConstructor) {
    return makeClass(
        "PassMultipleTextureNode", 3, baseConstructor,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 0, nullptr), JSValue(nullptr)});
            self.set("textureName", arg(args, 1, nullptr));
            self.set("previousTexture", arg(args, 2, JSValue(false)));
            self.set("isPassMultipleTextureNode", true);
            return self;
        },
        {}, "", "");
}

JSValue isCacheable(const JSValue&, const Args&, const Args&) { return JSValue(true); }

// isCacheable( /*builder*/ ) { return false; }
JSValue passIsCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// setResolutionScale( resolutionScale ) { this._resolutionScale = resolutionScale; return this; }
JSValue passSetResolutionScale(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    self.set("_resolutionScale", arg(args, 0, JSValue(1)));
    return self;
}
JSValue passGetResolutionScale(const JSValue& rawSelf, const Args&, const Args&) {
    return const_cast<JSValue&>(rawSelf).get("_resolutionScale");
}

// setLayers( layers ) { this._layers = layers; return this; }
JSValue passSetLayers(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    self.set("_layers", arg(args, 0, nullptr));
    return self;
}
JSValue passGetLayers(const JSValue& rawSelf, const Args&, const Args&) {
    return const_cast<JSValue&>(rawSelf).get("_layers");
}

// setMRT( mrt ) { this._mrt = mrt; return this; }
JSValue passSetMRT(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    self.set("_mrt", arg(args, 0, nullptr));
    return self;
}
JSValue passGetMRT(const JSValue& rawSelf, const Args&, const Args&) {
    return const_cast<JSValue&>(rawSelf).get("_mrt");
}

// getTexture( name ) { let texture = this._textures[ name ]; if ( texture === undefined ) { ... clone ... } return texture; }
JSValue passGetTexture(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto name = arg(args, 0);
    auto textures = self.get("_textures");
    auto texture = textures.get(name);
    if (!texture.toBoolean()) {
        if (name.toString() == "depth") {
            throw JSThrownValue(nativeGlobalBinding("Error")
                                    .construct({"THREE.PassNode: Depth texture is not available for this pass."}));
        }
        const auto renderTarget = self.get("renderTarget");
        texture = renderTarget.get("texture").get("clone").call(renderTarget.get("texture"), {});
        texture.set("name", name);
        textures.set(name, texture);
        self.set("_textures", textures);
        auto targetTextures = renderTarget.get("textures");
        targetTextures.get("push").call(targetTextures, {texture});
    }
    return texture;
}

// getPreviousTexture( name ) { let texture = this._previousTextures[ name ]; if ( texture === undefined ) {
//     texture = this.getTexture( name ).clone(); this._previousTextures[ name ] = texture; } return texture; }
JSValue passGetPreviousTexture(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto name = arg(args, 0);
    auto textures = self.get("_previousTextures");
    auto texture = textures.get(name);
    if (!texture.toBoolean()) {
        const auto source = self.get("getTexture").call(self, {name});
        texture = source.get("clone").call(source, {});
        textures.set(name, texture);
        self.set("_previousTextures", textures);
    }
    return texture;
}

// toggleTexture( name ) { const prevTexture = this._previousTextures[ name ]; if ( prevTexture !== undefined ) { ...swap... } }
JSValue passToggleTexture(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto name = arg(args, 0);
    auto previousTextures = self.get("_previousTextures");
    const auto previousTexture = previousTextures.get(name);
    if (previousTexture.toBoolean()) {
        auto textures = self.get("_textures");
        const auto texture = textures.get(name);
        const auto renderTarget = self.get("renderTarget");
        auto targetTextures = renderTarget.get("textures");
        const auto index = targetTextures.get("indexOf").call(targetTextures, {texture});
        targetTextures.set(index, previousTexture);
        textures.set(name, previousTexture);
        previousTextures.set(name, texture);
        self.set("_textures", textures);
        self.set("_previousTextures", previousTextures);
        const auto textureNodes = self.get("_textureNodes");
        const auto textureNode = textureNodes.get(name);
        if (textureNode.toBoolean()) textureNode.get("updateTexture").call(textureNode, {});
        const auto previousNodes = self.get("_previousTextureNodes");
        const auto previousNode = previousNodes.get(name);
        if (previousNode.toBoolean()) previousNode.get("updateTexture").call(previousNode, {});
    }
    return JSValue{};
}

// getTextureNode( name = 'output' ) { ... new PassMultipleTextureNode( this, name ) ... }
JSValue passGetTextureNode(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto name = arg(args, 0, JSValue("output"));
    auto nodes = self.get("_textureNodes");
    auto textureNode = nodes.get(name);
    if (!textureNode.toBoolean()) {
        textureNode = nativePassMultipleTextureNodeConstructor().construct({self, name});
        textureNode.get("updateTexture").call(textureNode, {});
        nodes.set(name, textureNode);
        self.set("_textureNodes", nodes);
    }
    return textureNode;
}

// getPreviousTextureNode( name = 'output' ) { ... if ( this._textureNodes[ name ] === undefined ) this.getTextureNode( name ); ... }
JSValue passGetPreviousTextureNode(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto name = arg(args, 0, JSValue("output"));
    auto nodes = self.get("_previousTextureNodes");
    auto textureNode = nodes.get(name);
    if (!textureNode.toBoolean()) {
        if (!self.get("_textureNodes").get(name).toBoolean()) {
            self.get("getTextureNode").call(self, {name});
        }
        textureNode = nativePassMultipleTextureNodeConstructor().construct({self, name, JSValue(true)});
        textureNode.get("updateTexture").call(textureNode, {});
        nodes.set(name, textureNode);
        self.set("_previousTextureNodes", nodes);
    }
    return textureNode;
}

// getViewZNode( name = 'depth' ) { ... perspectiveDepthToViewZ( getTextureNode( name ), near, far ) ... }
JSValue passGetViewZNode(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto name = arg(args, 0, JSValue("depth"));
    auto nodes = self.get("_viewZNodes");
    auto viewZNode = nodes.get(name);
    if (!viewZNode.toBoolean()) {
        const auto textureNode = self.get("getTextureNode").call(self, {name});
        viewZNode = nativeViewportDepthNodeBinding("perspectiveDepthToViewZ")
                        .call({}, {textureNode, self.get("_cameraNear"), self.get("_cameraFar")});
        nodes.set(name, viewZNode);
        self.set("_viewZNodes", nodes);
    }
    return viewZNode;
}

// getLinearDepthNode( name = 'depth' ) { ... viewZToOrthographicDepth( getViewZNode( name ), near, far ) ... }
JSValue passGetLinearDepthNode(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto name = arg(args, 0, JSValue("depth"));
    auto nodes = self.get("_linearDepthNodes");
    auto linearDepthNode = nodes.get(name);
    if (!linearDepthNode.toBoolean()) {
        const auto viewZNode = self.get("getViewZNode").call(self, {name});
        linearDepthNode = nativeViewportDepthNodeBinding("viewZToOrthographicDepth")
                              .call({}, {viewZNode, self.get("_cameraNear"), self.get("_cameraFar")});
        nodes.set(name, linearDepthNode);
        self.set("_linearDepthNodes", nodes);
    }
    return linearDepthNode;
}

// setup( { renderer } ) { renderTarget.samples = options.samples ?? renderer.samples; ... }
JSValue passSetup(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto renderer = arg(args, 0).get("renderer");
    auto renderTarget = self.get("renderTarget");
    const auto options = self.get("options");
    const auto samplesOption = options.isObject() ? options.get("samples") : JSValue{};
    renderTarget.set("samples",
                     samplesOption.isUndefined() ? renderer.get("samples") : samplesOption);
    renderTarget.get("texture").set("type", renderer.get("getOutputBufferType").call(renderer, {}));
    auto depthTexture = renderTarget.get("depthTexture");
    if (renderer.get("reversedDepthBuffer").strictEquals(true) && depthTexture.toBoolean()) {
        depthTexture.set("type", JSValue(static_cast<double>(FloatType)));
    }
    if (self.get("scope").strictEquals(JSValue("color"))) {
        return self.get("getTextureNode").call(self, {});
    }
    return self.get("getLinearDepthNode").call(self, {});
}

// setSize( width, height ) { ... effective = floor( size * resolutionScale ); renderTarget.setSize(...); scissor/viewport }
JSValue passSetSize(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto width = arg(args, 0, JSValue(1));
    const auto height = arg(args, 1, JSValue(1));
    self.set("_width", width);
    self.set("_height", height);
    const auto scale = self.get("_resolutionScale").toNumber();
    const auto effectiveWidth = JSValue(std::floor(width.toNumber() * scale));
    const auto effectiveHeight = JSValue(std::floor(height.toNumber() * scale));
    auto renderTarget = self.get("renderTarget");
    renderTarget.get("setSize").call(renderTarget, {effectiveWidth, effectiveHeight});

    auto scissor = self.get("_scissor");
    if (scissor.toBoolean()) {
        auto targetScissor = renderTarget.get("scissor");
        targetScissor = targetScissor.get("copy").call(targetScissor, {scissor});
        targetScissor = targetScissor.get("multiplyScalar").call(targetScissor, {JSValue(scale)});
        targetScissor.get("floor").call(targetScissor, {});
        renderTarget.set("scissor", targetScissor);
        renderTarget.set("scissorTest", true);
    } else {
        renderTarget.set("scissorTest", false);
    }

    auto viewport = self.get("_viewport");
    if (viewport.toBoolean()) {
        auto targetViewport = renderTarget.get("viewport");
        targetViewport = targetViewport.get("copy").call(targetViewport, {viewport});
        targetViewport = targetViewport.get("multiplyScalar").call(targetViewport, {JSValue(scale)});
        targetViewport.get("floor").call(targetViewport, {});
        renderTarget.set("viewport", targetViewport);
    }
    return JSValue{};
}

JSValue passSetScissor(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto x = arg(args, 0, JSValue(nullptr));
    if (x.isNull()) {
        self.set("_scissor", JSValue(nullptr));
        return JSValue{};
    }
    auto scissor = self.get("_scissor");
    if (!scissor.toBoolean()) scissor = nativeVector4Constructor().construct();
    if (x.get("isVector4").toBoolean()) {
        scissor.get("copy").call(scissor, {x});
    } else {
        scissor.get("set").call(scissor, {x, arg(args, 1, JSValue(0)),
                                          arg(args, 2, JSValue(0)), arg(args, 3, JSValue(0))});
    }
    self.set("_scissor", scissor);
    return JSValue{};
}

JSValue passSetViewport(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto x = arg(args, 0, JSValue(nullptr));
    if (x.isNull()) {
        self.set("_viewport", JSValue(nullptr));
        return JSValue{};
    }
    auto viewport = self.get("_viewport");
    if (!viewport.toBoolean()) viewport = nativeVector4Constructor().construct();
    if (x.get("isVector4").toBoolean()) {
        viewport.get("copy").call(viewport, {x});
    } else {
        viewport.get("set").call(viewport, {x, arg(args, 1, JSValue(0)),
                                              arg(args, 2, JSValue(0)), arg(args, 3, JSValue(0))});
    }
    self.set("_viewport", viewport);
    return JSValue{};
}

// dispose() { super.dispose(); this.renderTarget.dispose(); }
JSValue passDispose(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto basePrototype = nativeNodeConstructor().get("prototype");
    const auto baseDispose = basePrototype.get("dispose");
    if (baseDispose.toBoolean()) baseDispose.call(self, {});
    self.get("renderTarget").get("dispose").call(self.get("renderTarget"), {});
    return JSValue{};
}

// updateBefore( frame ) { save the renderer and scene state, render the scene into the pass target, restore. }
JSValue passUpdateBefore(const JSValue& rawSelf, const Args& args, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto frame = arg(args, 0);
    auto renderer = frame.get("renderer");
    auto scene = self.get("scene");

    auto camera = JSValue{};
    auto size = nativeVector2Constructor().construct();
    const auto outputRenderTarget = renderer.get("getOutputRenderTarget").call(renderer, {});
    if (outputRenderTarget.toBoolean() &&
        outputRenderTarget.get("isXRRenderTarget").strictEquals(true)) {
        const auto xr = renderer.get("xr");
        camera = xr.get("getCamera").call(xr, {});
        xr.get("updateCamera").call(xr, {camera});
        size.get("set").call(size, {outputRenderTarget.get("width"), outputRenderTarget.get("height")});
    } else {
        camera = self.get("camera");
        size = renderer.get("getDrawingBufferSize").call(renderer, {size});
    }
    self.get("setSize").call(self, {size.get("width"), size.get("height")});

    const auto currentRenderTarget = renderer.get("getRenderTarget").call(renderer, {});
    const auto currentMRT = renderer.get("getMRT").call(renderer, {});
    const auto currentAutoClear = renderer.get("autoClear");
    const auto currentAutoClearColor = renderer.get("autoClearColor");
    const auto currentAutoClearDepth = renderer.get("autoClearDepth");
    const auto currentAutoClearStencil = renderer.get("autoClearStencil");
    const auto currentTransparent = renderer.get("transparent");
    const auto currentOpaque = renderer.get("opaque");
    const auto currentLighting = renderer.get("lighting");
    auto cameraLayers = camera.get("layers");
    const auto currentMask = cameraLayers.get("mask");
    const auto currentContextNode = renderer.get("contextNode");
    const auto currentOverrideMaterial = scene.get("overrideMaterial");

    auto nearUniform = self.get("_cameraNear");
    nearUniform.set("value", camera.get("near"));
    auto farUniform = self.get("_cameraFar");
    farUniform.set("value", camera.get("far"));

    const auto layers = self.get("_layers");
    if (layers.toBoolean()) {
        cameraLayers.set("mask", layers.get("mask"));
        camera.set("layers", cameraLayers);
    }

    auto previousTextures = self.get("_previousTextures");
    const auto keys = nativeGlobalBinding("Object").get("keys").call({}, {previousTextures});
    const auto keysLength = keys.get("length").toNumber();
    for (double index = 0; index < keysLength; ++index) {
        self.get("toggleTexture").call(self, {keys.get(JSValue(index))});
    }

    const auto overrideMaterial = self.get("overrideMaterial");
    if (overrideMaterial.toBoolean()) scene.set("overrideMaterial", overrideMaterial);

    renderer.get("setRenderTarget").call(renderer, {self.get("renderTarget")});
    renderer.get("setMRT").call(renderer, {self.get("_mrt")});
    renderer.set("autoClear", self.get("autoClear"));
    renderer.set("autoClearColor", self.get("autoClearColor"));
    renderer.set("autoClearDepth", self.get("autoClearDepth"));
    renderer.set("autoClearStencil", self.get("autoClearStencil"));
    renderer.set("transparent", self.get("transparent"));
    renderer.set("opaque", self.get("opaque"));
    const auto lighting = self.get("lighting");
    if (lighting.toBoolean()) renderer.set("lighting", lighting);

    const auto contextNode = self.get("contextNode");
    if (contextNode.toBoolean()) {
        auto cache = self.get("_contextNodeCache");
        const auto version = self.get("version");
        if (!cache.toBoolean() || !cache.get("version").strictEquals(version)) {
            const auto rendererContext = renderer.get("contextNode");
            auto flowData = JSValue::object(nullptr);
            if (rendererContext.toBoolean()) {
                flowData = nativeGlobalBinding("Object").get("assign").call(
                    {}, {flowData, rendererContext.get("getFlowContextData").call(rendererContext, {})});
            }
            flowData = nativeGlobalBinding("Object").get("assign").call(
                {}, {flowData, contextNode.get("getFlowContextData").call(contextNode, {})});
            cache = JSValue::object(nullptr);
            cache.set("version", version);
            cache.set("context", nativeTSLBaseBinding("context").call({}, {flowData}));
            self.set("_contextNodeCache", cache);
        }
        renderer.set("contextNode", cache.get("context"));
    }

    const auto currentSceneName = scene.get("name");
    const auto passName = self.get("name");
    scene.set("name", passName.toBoolean() ? passName : currentSceneName);
    renderer.get("render").call(renderer, {scene, camera});
    scene.set("name", currentSceneName);
    scene.set("overrideMaterial", currentOverrideMaterial);

    renderer.get("setRenderTarget").call(renderer, {currentRenderTarget});
    renderer.get("setMRT").call(renderer, {currentMRT});
    renderer.set("autoClear", currentAutoClear);
    renderer.set("autoClearColor", currentAutoClearColor);
    renderer.set("autoClearDepth", currentAutoClearDepth);
    renderer.set("autoClearStencil", currentAutoClearStencil);
    renderer.set("transparent", currentTransparent);
    renderer.set("opaque", currentOpaque);
    renderer.set("contextNode", currentContextNode);
    renderer.set("lighting", currentLighting);

    cameraLayers.set("mask", currentMask);
    camera.set("layers", cameraLayers);
    return JSValue{};
}

// constructor( scope, scene, camera, options = {} ) { super( 'vec4' ); ... }
JSValue passNodeInitialize(const JSValue& target, const Args& args, const Args& captures) {
    auto self = superConstruct(target, captures, {JSValue("vec4")});
    const auto scope = arg(args, 0, nullptr);
    const auto scene = arg(args, 1, nullptr);
    const auto camera = arg(args, 2, nullptr);
    const auto options = arg(args, 3, JSValue::object(nullptr));

    self.set("scope", scope);
    self.set("scene", scene);
    self.set("camera", camera);
    self.set("options", options);
    self.set("_width", JSValue(1));
    self.set("_height", JSValue(1));

    auto defaults = JSValue::object(nullptr);
    defaults.set("type", JSValue(static_cast<double>(HalfFloatType)));
    auto renderTargetOptions =
        nativeGlobalBinding("Object").get("assign").call({}, {JSValue::object(nullptr), defaults, options});
    auto renderTarget = nativeRenderTargetConstructor().construct(
        {JSValue(1), JSValue(1), renderTargetOptions});
    renderTarget.get("texture").set("name", JSValue("output"));

    auto depthTexture = JSValue{};
    const bool depthScope = scope.strictEquals(JSValue("depth"));
    const auto depthBuffer = options.isObject() ? options.get("depthBuffer") : JSValue{};
    if (depthScope || !depthBuffer.strictEquals(JSValue(false))) {
        const auto providedDepth = options.isObject() ? options.get("depthTexture") : JSValue{};
        depthTexture = providedDepth.isUndefined()
                           ? nativeDepthTextureConstructor().construct()
                           : providedDepth;
        if (renderTarget.get("stencilBuffer").strictEquals(true)) {
            depthTexture.set("format", JSValue(static_cast<double>(DepthStencilFormat)));
            depthTexture.set("type", JSValue(static_cast<double>(UnsignedInt248Type)));
        }
        depthTexture.set("isRenderTargetTexture", true);
        depthTexture.set("name", JSValue("depth"));
        renderTarget.set("depthTexture", depthTexture);
    }

    self.set("renderTarget", renderTarget);
    self.set("overrideMaterial", JSValue(nullptr));
    self.set("transparent", true);
    self.set("opaque", true);
    self.set("lighting", JSValue(nullptr));
    const auto optionOr = [&options](const char* name) {
        const auto value = options.isObject() ? options.get(name) : JSValue{};
        return value.isUndefined() ? JSValue(true) : value;
    };
    self.set("autoClear", optionOr("autoClear"));
    self.set("autoClearColor", optionOr("autoClearColor"));
    self.set("autoClearDepth", optionOr("autoClearDepth"));
    self.set("autoClearStencil", optionOr("autoClearStencil"));
    self.set("contextNode", JSValue(nullptr));
    self.set("_contextNodeCache", JSValue(nullptr));

    auto textures = JSValue::object(nullptr);
    textures.set("output", renderTarget.get("texture"));
    if (depthTexture.toBoolean()) textures.set("depth", depthTexture);
    self.set("_textures", textures);
    self.set("_textureNodes", JSValue::object(nullptr));
    self.set("_linearDepthNodes", JSValue::object(nullptr));
    self.set("_viewZNodes", JSValue::object(nullptr));
    self.set("_previousTextures", JSValue::object(nullptr));
    self.set("_previousTextureNodes", JSValue::object(nullptr));
    self.set("_cameraNear", nativeUniformFactory().call({}, {JSValue(0.0)}));
    self.set("_cameraFar", nativeUniformFactory().call({}, {JSValue(0.0)}));
    self.set("_mrt", JSValue(nullptr));
    self.set("_layers", JSValue(nullptr));
    self.set("_resolutionScale", JSValue(1));
    self.set("_viewport", JSValue(nullptr));
    self.set("_scissor", JSValue(nullptr));
    self.set("isPassNode", true);
    self.set("updateBeforeType", nativeNodeConstantsBinding("NodeUpdateType").get("FRAME"));
    self.set("global", true);
    return self;
}

// updateTexture() { this.value = this.previousTexture ? this.passNode.getPreviousTexture( this.textureName )
//                                                    : this.passNode.getTexture( this.textureName ); }
JSValue passMultipleTextureUpdate(const JSValue& rawSelf, const Args&, const Args&) {
    JSValue& self = const_cast<JSValue&>(rawSelf);
    const auto passNode = self.get("passNode");
    const auto textureName = self.get("textureName");
    JSValue value;
    if (self.get("previousTexture").toBoolean()) {
        value = passNode.get("getPreviousTexture").call(passNode, {textureName});
    } else {
        value = passNode.get("getTexture").call(passNode, {textureName});
    }
    self.set("value", value);
    return {};
}
}  // namespace

JSValue nativePassNodeModule() {
    using namespace LightingNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "PassNode module");
    nativeTextureNodeModule();

    const auto textureConstructor = nativeTextureNodeConstructor();
    const auto texturePrototype = textureConstructor.get("prototype");
    JSValue bindings = JSValue::object(nullptr);

    const auto passTextureConstructor =
        passTextureNodeConstructor(texturePrototype, textureConstructor);
    {
        auto prototype = passTextureConstructor.get("prototype");
        prototype.setPrototype(texturePrototype);
        method(prototype, "isCacheable", 0, isCacheable, {}, "");
        auto setup = JSValue::function(
            +[](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
                const auto builder = arg(args, 0);
                auto properties = builder.get("getNodeProperties").call(builder, {self});
                properties.set("passNode", self.get("passNode"));
                const auto basePrototype = captures.at(0);
                const auto baseSetup = basePrototype.get("setup");
                if (baseSetup.toBoolean()) return baseSetup.call(self, {builder});
                return self;
            },
            {texturePrototype});
        prototype.set("setup", setup);
        own(bindings, "PassTextureNode", passTextureConstructor);
    }

    const auto passMultipleConstructor =
        passMultipleTextureNodeConstructor(passTextureConstructor);
    {
        auto prototype = passMultipleConstructor.get("prototype");
        prototype.setPrototype(passTextureConstructor.get("prototype"));
        method(prototype, "updateTexture", 0, passMultipleTextureUpdate, {}, "");
        own(bindings, "PassMultipleTextureNode", passMultipleConstructor);
    }

    const auto nodeBase = nativeNodeConstructor();
    auto passNodeConstructor =
        makeClass("PassNode", 4, nodeBase, passNodeInitialize, {}, "", "");
    const auto defineProperty = nativeGlobalBinding("Object").get("defineProperty");
    for (const auto& [getterName, value] : std::initializer_list<std::pair<const char*, const char*>>{
             {"COLOR", "color"}, {"DEPTH", "depth"}}) {
        auto descriptor = JSValue::object(nullptr);
        // One getter per scope value, registered as an accessor on the constructor like the pinned statics.
        auto getter = JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue {
                return captures.at(0);
            },
            {JSValue(value)});
        descriptor.set("get", getter);
        descriptor.set("enumerable", false);
        descriptor.set("configurable", false);
        defineProperty.call({}, {passNodeConstructor, JSValue(getterName), descriptor});
    }
    {
        const auto prototype = passNodeConstructor.get("prototype");
        method(prototype, "isCacheable", 0, passIsCacheable, {}, "");
        method(prototype, "setResolutionScale", 1, passSetResolutionScale, {}, "");
        method(prototype, "getResolutionScale", 0, passGetResolutionScale, {}, "");
        method(prototype, "setLayers", 1, passSetLayers, {}, "");
        method(prototype, "getLayers", 0, passGetLayers, {}, "");
        method(prototype, "setMRT", 1, passSetMRT, {}, "");
        method(prototype, "getMRT", 0, passGetMRT, {}, "");
        method(prototype, "getTexture", 1, passGetTexture, {}, "");
        method(prototype, "getPreviousTexture", 1, passGetPreviousTexture, {}, "");
        method(prototype, "toggleTexture", 1, passToggleTexture, {}, "");
        method(prototype, "getTextureNode", 1, passGetTextureNode, {}, "");
        method(prototype, "getPreviousTextureNode", 1, passGetPreviousTextureNode, {}, "");
        method(prototype, "getViewZNode", 1, passGetViewZNode, {}, "");
        method(prototype, "getLinearDepthNode", 1, passGetLinearDepthNode, {}, "");
        method(prototype, "setup", 1, passSetup, {}, "");
        method(prototype, "setSize", 2, passSetSize, {}, "");
        method(prototype, "setScissor", 4, passSetScissor, {}, "");
        method(prototype, "setViewport", 4, passSetViewport, {}, "");
        method(prototype, "dispose", 0, passDispose, {}, "");
        method(prototype, "updateBefore", 1, passUpdateBefore, {}, "");
    }
    own(bindings, "PassNode", passNodeConstructor);

    // pass( scene, camera, options ) => new PassNode( PassNode.COLOR, scene, camera, options )
    auto makePassFactory = [&](const char* scopeValue, const char* exportName) {
        auto factory = JSValue::function(
            +[](const JSValue&, const Args& factoryArgs, const Args& captures) -> JSValue {
                return captures.at(0).construct(
                    {captures.at(1), arg(factoryArgs, 0, nullptr), arg(factoryArgs, 1, nullptr),
                     arg(factoryArgs, 2, JSValue::object(nullptr))});
            },
            {passNodeConstructor, JSValue(scopeValue)});
        own(bindings, exportName, factory);
    };
    makePassFactory("color", "pass");
    makePassFactory("depth", "depthPass");

    // passTexture( pass, texture ) => new PassTextureNode( pass, texture )
    own(bindings, "passTexture",
        JSValue::function(
            +[](const JSValue&, const Args& factoryArgs, const Args& captures) -> JSValue {
                return captures.at(0).construct(
                    {arg(factoryArgs, 0, nullptr), arg(factoryArgs, 1, nullptr)});
            },
            {passTextureConstructor}));

    return publish(bindings, lexical(), weak);
}

JSValue nativePassNodeBinding(const std::string& name) {
    const auto keeper = nativePassNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativePassNodeConstructor() { return nativePassNodeBinding("PassNode"); }
JSValue nativePassTextureNodeConstructor() { return nativePassNodeBinding("PassTextureNode"); }
JSValue nativePassMultipleTextureNodeConstructor() {
    return nativePassNodeBinding("PassMultipleTextureNode");
}