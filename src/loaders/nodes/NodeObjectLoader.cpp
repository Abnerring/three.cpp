#include "loaders/nodes/NodeObjectLoader.h"
#include "loaders/nodes/NodeLoader.h"
#include "loaders/nodes/NodeMaterialLoader.h"
#include "loaders/ObjectLoader.h"
#include "core/JSAsyncFunction.h"
#include "core/JSModuleObject.h"
#include "core/JSOperators.h"
#include "loaders/nodes/NodeObjectLoaderSources.inc"
#include <initializer_list>
#include <tuple>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue{};
}

JSValue setNodes(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    self.set("nodes", arg(args, 0));
    return self;
}

JSValue setNodeMaterials(const JSValue& receiver, const Args& args, const Args&) {
    auto self = receiver;
    self.set("nodeMaterials", arg(args, 0));
    return self;
}

JSValue parse(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto json = arg(args, 0);
    self.set("_nodesJSON", json.get("nodes"));
    const auto base = captures[0].getPrototype();
    const auto method = base.get("parse", self);
    const auto result = method.call(self, {json, arg(args, 1)});
    self.set("_nodesJSON", nullptr);
    return result;
}

JSAsyncStep parseAsync(const JSValue& state, const JSAsyncCompletion& completion,
                       const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    if (state.get("awaiting").isUndefined()) {
        const auto json = arg(args, 0);
        self.set("_nodesJSON", json.get("nodes"));
        const auto base = captures[0].getPrototype();
        const auto method = base.get("parseAsync", self);
        const auto result = method.call(self, {json});
        auto frame = state;
        frame.set("awaiting", true);
        return JSAsyncStep::awaitValue(result);
    }
    if (completion.kind == JSAsyncCompletionKind::Throw)
        return JSAsyncStep::abrupt(completion.value);
    self.set("_nodesJSON", nullptr);
    return JSAsyncStep::complete(completion.value);
}

JSValue parseNodes(const JSValue& self, const Args& args, const Args&) {
    const auto json = arg(args, 0);
    if (!json.isUndefined()) {
        const auto loader = nativeNodeLoaderConstructor().construct();
        const auto setNodes = loader.get("setNodes");
        setNodes.call(loader, {self.get("nodes")});
        const auto setTextures = loader.get("setTextures");
        setTextures.call(loader, {arg(args, 1)});
        const auto parse = loader.get("parseNodes");
        return parse.call(loader, {json});
    }
    return JSValue::object();
}

JSValue parseMaterials(const JSValue& self, const Args& args, const Args&) {
    auto materials = JSValue::object();
    const auto json = arg(args, 0), textures = arg(args, 1);
    if (!json.isUndefined()) {
        const auto parseNodes = self.get("parseNodes");
        const auto nodes = parseNodes.call(self, {self.get("_nodesJSON"), textures});
        const auto loader = nativeNodeMaterialLoaderConstructor().construct();
        const auto setTextures = loader.get("setTextures");
        setTextures.call(loader, {textures});
        const auto setNodes = loader.get("setNodes");
        setNodes.call(loader, {nodes});
        const auto setNodeMaterials = loader.get("setNodeMaterials");
        setNodeMaterials.call(loader, {self.get("nodeMaterials")});
        const auto length = json.get("length");
        for (double i = 0; JSOperators::less(i, length); ++i) {
            const auto data = json.get(JSValue(i));
            const auto key = data.get("uuid");
            const auto parse = loader.get("parse");
            const auto material = parse.call(loader, {data});
            materials.set(key, material);
        }
    }
    return materials;
}
}

JSValue nativeNodeObjectLoaderModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeObjectLoader module");
    nativeNodeLoaderModule();
    nativeNodeMaterialLoaderModule();
    const auto base = nativeObjectLoaderConstructor();
    auto holder = JSValue::object(nullptr);
    auto constructor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            const auto base = captures[0].get("constructor").getPrototype();
            auto self = base.construct({arg(args, 0)}, target);
            self.set("nodes", JSValue::object());
            self.set("nodeMaterials", JSValue::object());
            self.set("_nodesJSON", nullptr);
            return self;
        }, {holder}, "NodeObjectLoader", 1);
    holder.set("constructor", constructor);
    constructor.setPrototype(base);
    constructor.setFunctionSourceText(NodeObjectLoaderSource::classSource);
    auto prototype = constructor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    for (const auto& [name, arity, callback, source] :
         std::initializer_list<std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {"setNodes", 1, setNodes, NodeObjectLoaderSource::setNodesSource},
             {"setNodeMaterials", 1, setNodeMaterials, NodeObjectLoaderSource::setNodeMaterialsSource},
             {"parse", 2, parse, NodeObjectLoaderSource::parseSource},
             {"parseAsync", 1, nullptr, NodeObjectLoaderSource::parseAsyncSource},
             {"parseNodes", 2, parseNodes, NodeObjectLoaderSource::parseNodesSource},
             {"parseMaterials", 2, parseMaterials, NodeObjectLoaderSource::parseMaterialsSource}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = callback ? JSValue::function(callback, {prototype}, name, arity)
                                    : createNativeAsyncStateFunction(parseAsync, {prototype}, name, arity, source);
        descriptor.value->setFunctionSourceText(source);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }
    auto module = JSValue::object(nullptr);
    module.set("default", constructor);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeNodeObjectLoaderBinding(const std::string& name) {
    return nativeNodeObjectLoaderModule().get(name);
}

JSValue nativeNodeObjectLoaderConstructor() {
    return nativeNodeObjectLoaderBinding("default");
}
