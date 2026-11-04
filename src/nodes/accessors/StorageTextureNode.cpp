#include "nodes/accessors/StorageTextureNode.h"

#include "nodes/accessors/TextureNode.h"
#include "nodes/core/constants.h"
#include "nodes/core/VarNode.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "textures/Texture.h"
#include "utils.h"

#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue baseMethod(const JSValue& self, const Args& args, const JSValue& prototype, const char* name) {
    return prototype.getPrototype().get(name, self).call(self, args);
}

JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto name = captures[0].string();
    const auto prototype = captures[1];
    const auto imports = captures[2];

    if (name == "getInputType") return "storageTexture";
    if (name == "getTransformedUV") return arg(args, 0);
    if (name == "setAccess") {
        self.set("access", arg(args, 0));
        return self;
    }
    if (name == "toReadWrite") return invoke(self, "setAccess")({imports.get("NodeAccess").get("READ_WRITE")});
    if (name == "toReadOnly") return invoke(self, "setAccess")({imports.get("NodeAccess").get("READ_ONLY")});
    if (name == "toWriteOnly") return invoke(self, "setAccess")({imports.get("NodeAccess").get("WRITE_ONLY")});
    if (name == "setMipLevel") {
        self.set("mipLevel", arg(args, 0));
        return self;
    }
    if (name == "generateSnippet") {
        const auto builder = arg(args, 0);
        return invoke(builder, "generateStorageTextureLoad")({self.get("value"), arg(args, 1), arg(args, 2), arg(args, 3), arg(args, 5), arg(args, 9)});
    }
    if (name == "generate") {
        if (!self.get("storeNode").isNull()) {
            invoke(self, "generateStore")({arg(args, 0)});
            return "";
        }
        return baseMethod(self, args, prototype, "generate");
    }
    if (name == "generateStore") {
        const auto builder = arg(args, 0);
        const auto properties = invoke(builder, "getNodeProperties")({self});
        const auto textureProperty = baseMethod(self, {builder, "property"}, prototype, "generate");
        const auto uvNode = properties.get("uvNode").toBoolean() ? properties.get("uvNode") : self.get("uvNode");
        const auto storeNode = properties.get("storeNode").toBoolean() ? properties.get("storeNode") : self.get("storeNode");
        const auto depthNode = properties.get("depthNode");
        const auto uvSnippet = invoke(uvNode, "build")({builder, self.get("value").get("is3DTexture").strictEquals(true) ? JSValue("uvec3") : JSValue("uvec2")});
        const auto storeType = invoke(builder, "getTypeFromLength")({4, invoke(builder, "getComponentTypeFromTexture")({self.get("value")})});
        const auto storeSnippet = invoke(storeNode, "build")({builder, storeType});
        const auto depthSnippet = depthNode.toBoolean() ? invoke(depthNode, "build")({builder, "int"}) : JSValue(nullptr);
        const auto snippet = invoke(builder, "generateTextureStore")({self.get("value"), textureProperty, uvSnippet, depthSnippet, storeSnippet});
        invoke(builder, "addLineFlowCode")({snippet, self});
        return {};
    }
    if (name == "store") {
        auto node = invoke(self, "clone")();
        node.set("referenceNode", invoke(self, "getBase")());
        node.set("uvNode", arg(args, 0));
        node.set("storeNode", arg(args, 1));
        if (!arg(args, 1).isNull()) invoke(node, "toStack")();
        return node;
    }
    if (name == "clone") {
        auto node = baseMethod(self, {}, prototype, "clone");
        node.set("storeNode", self.get("storeNode"));
        node.set("mipLevel", self.get("mipLevel"));
        node.set("access", self.get("access"));
        return node;
    }
    if (name == "textureStore") {
        const auto value = arg(args, 0), uv = arg(args, 1), store = arg(args, 2);
        if (value.get("isStorageTextureNode").strictEquals(true)) return invoke(value, "store")({uv, store});
        auto node = imports.get("storageTexture").call({}, {value, uv, store});
        if (!store.isNull()) invoke(node, "toStack")();
        return node;
    }
    throw std::logic_error("Unknown StorageTextureNode method: " + name);
}

JSValue storageFactory(const JSValue&, const Args& args, const Args& captures) {
    const auto ctor = captures[0];
    const auto intent = nativeVarNodeBinding("VarIntent");
    const auto toIntent = [&](const JSValue& value) {
        return value.isNull() || value.isUndefined() ? value : intent.call({}, {value});
    };
    return ctor.construct({arg(args, 0), toIntent(arg(args, 1, nullptr)), toIntent(arg(args, 2, nullptr))});
}

JSValue textureStoreFactory(const JSValue&, const Args& args, const Args& captures) {
    const auto value = arg(args, 0), uv = arg(args, 1), store = arg(args, 2);
    if (value.get("isStorageTextureNode").strictEquals(true)) return invoke(value, "store")({uv, store});
    auto node = captures[0].call({}, {value, uv, store});
    if (!store.isNull()) invoke(node, "toStack")();
    return node;
}
} // namespace

JSValue nativeStorageTextureNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "StorageTextureNode module");

    const auto base = nativeTextureNodeConstructor();
    // TextureNode initializes the TSL realm; install VarNode's intent
    // conversion afterward so nodeProxy arguments match three.js objects.
    nativeVarNodeModule();
    const auto access = nativeNodeConstantsBinding("NodeAccess");
    auto imports = JSValue::object(nullptr);
    own(imports, "NodeAccess", access);
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto value = arg(args, 0);
            auto self = superConstruct(target, captures, {value, arg(args, 1, nullptr)});
            self.set("isStorageTextureNode", true);
            self.set("storeNode", arg(args, 2, nullptr));
            self.set("mipLevel", 0);
            self.set("access", captures[1].get("NodeAccess").get("WRITE_ONLY"));
            return self;
        },
        {holder, imports}, "StorageTextureNode", 1);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type;
    type.configurable = true;
    type.getter = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return "StorageTextureNode"; }, {}, "get type", 0);
    ctor.defineProperty("type", type);

    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             {"getInputType", 0}, {"getTransformedUV", 1}, {"setAccess", 1}, {"toReadWrite", 0},
             {"toReadOnly", 0}, {"toWriteOnly", 0}, {"setMipLevel", 1}, {"generate", 2},
             {"generateSnippet", 9}, {"generateStore", 1}, {"store", 2}, {"clone", 0}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(method, {name, prototype, imports}, name, arity);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    auto storage = JSValue::function(storageFactory, {ctor}, "storageTexture", 1);
    own(bindings, "storageTexture", storage);
    auto textureStore = JSValue::function(textureStoreFactory, {storage, imports}, "textureStore", 3);
    own(bindings, "textureStore", textureStore);
    auto module = JSValue::object(nullptr);
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeStorageTextureNodeBinding(const std::string& name) {
    const auto keeper = nativeStorageTextureNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeStorageTextureNodeConstructor() { return nativeStorageTextureNodeBinding("default"); }
