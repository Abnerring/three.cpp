#include "nodes/display/PassNode.h"

#include "core/JSWeakMap.h"
#include "nodes/accessors/TextureNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"

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