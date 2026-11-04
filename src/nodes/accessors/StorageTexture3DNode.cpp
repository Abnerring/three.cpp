// Native port of three.js StorageTexture3DNode.js.
#include "nodes/accessors/StorageTexture3DNode.h"

#include "nodes/accessors/StorageTextureNode.h"
#include "nodes/core/VarNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <algorithm>
#include <initializer_list>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    const auto self = receiver;
    const auto name = captures[0].string();
    const auto vec3 = captures[2];

    if (name == "getDefaultUV") return vec3.call({}, {.5, .5, .5});
    if (name == "setUpdateMatrix") return {};
    if (name == "generateUV") {
        const auto builder = arg(args, 0);
        const auto uv = arg(args, 1);
        return invoke(uv, "build")({builder, self.get("sampler").strictEquals(true) ? JSValue("vec3") : JSValue("ivec3")});
    }
    if (name == "generateOffset") return invoke(arg(args, 1), "build")({arg(args, 0), "ivec3"});
    throw std::logic_error("Unknown StorageTexture3DNode method: " + name);
}

JSValue factory(const JSValue&, const Args& args, const Args& captures) {
    const auto intent = nativeVarNodeBinding("VarIntent");
    const auto toIntent = [&](const JSValue& value) {
        return value.isNull() || value.isUndefined() ? value : intent.call({}, {value});
    };
    return captures[0].construct({arg(args, 0), toIntent(arg(args, 1, nullptr)), toIntent(arg(args, 2, nullptr))});
}
} // namespace

JSValue nativeStorageTexture3DNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "StorageTexture3DNode module");

    const auto base = nativeStorageTextureNodeConstructor();
    nativeVarNodeModule();
    const auto vec3 = nativeTSLBaseBinding("vec3");

    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures, {arg(args, 0), arg(args, 1, nullptr), arg(args, 2, nullptr)});
            self.set("isStorageTexture3DNode", true);
            return self;
        },
        {holder}, "StorageTexture3DNode", 1);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));

    JSPropertyDescriptor type;
    type.configurable = true;
    type.getter = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return "StorageTexture3DNode"; }, {}, "get type", 0);
    ctor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             {"getDefaultUV", 0}, {"setUpdateMatrix", 1}, {"generateUV", 2}, {"generateOffset", 2}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(method, {name, prototype, vec3}, name, arity);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    own(bindings, "storageTexture3D", JSValue::function(factory, {ctor}, "storageTexture3D", 1));
    auto module = JSValue::object(nullptr);
    auto names = bindings.ownPropertyNames();
    std::sort(names.begin(), names.end());
    for (const auto& name : names) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeStorageTexture3DNodeBinding(const std::string& name) {
    const auto keeper = nativeStorageTexture3DNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeStorageTexture3DNodeConstructor() { return nativeStorageTexture3DNodeBinding("default"); }

StorageTexture3DNode::StorageTexture3DNode(const JSValue& value, const JSValue& uvNode, const JSValue& storeNode)
    : object_(constructor().construct({value, uvNode, storeNode})) {}
