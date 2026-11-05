#include "nodes/display/PassNode.h"

#include "core/JSWeakMap.h"
#include "nodes/accessors/TextureNode.h"
#include "constants.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "nodes/core/Node.h"
#include "nodes/core/UniformNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "core/RenderTarget.h"
#include "textures/DepthTexture.h"

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