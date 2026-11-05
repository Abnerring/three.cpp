#include "loaders/nodes/NodeLoader.h"

#include "loaders/nodes/NodeLoaderSources.inc"

#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "loaders/FileLoader.h"
#include "loaders/Loader.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "utils.h"

#include <string>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// load( url, onLoad, onProgress, onError ) { ... }
JSValue load(const JSValue& self, const Args& args, const Args&) {
    const auto url = arg(args, 0);
    const auto onLoad = arg(args, 1);
    const auto onProgress = arg(args, 2);
    const auto onError = arg(args, 3);

    FileLoader fileLoader(self.get("manager"));
    invoke(fileLoader.object(), "setPath")({self.get("path")});
    invoke(fileLoader.object(), "setRequestHeader")({self.get("requestHeader")});
    invoke(fileLoader.object(), "setWithCredentials")({self.get("withCredentials")});

    // ( text ) => { try { onLoad( this.parse( JSON.parse( text ) ) ); } catch ( e ) { ... } }
    auto onText = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto self = captures[0];
            const auto text = arg(args, 0);
            const auto onLoad = captures[1];
            const auto onError = captures[2];
            const auto url = captures[3];
            try {
                const auto parse = invoke(self, "parse");
                const auto jsonObject = nativeGlobalBinding("JSON");
                const auto json = jsonObject.get("parse").call(jsonObject, {text});
                onLoad.call({}, {parse({json})});
            } catch (...) {
                const auto exception = nativeExceptionValue(std::current_exception());
                if (onError.toBoolean()) {
                    onError.call({}, {exception});
                } else {
                    ThreeUtils::error({exception});
                }
                invoke(self.get("manager"), "itemError")({url});
            }
            return {};
        },
        {self, onLoad, onError, url}, "", 1);
    onText.setFunctionSourceText(NodeLoaderSource::loadCallbackSource);
    fileLoader.load(url, onText, onProgress, onError);
    return {};
}

// parseNodes( json ) { ... }
JSValue parseNodes(const JSValue& self, const Args& args, const Args&) {
    auto nodes = JSValue::object();
    const auto json = arg(args, 0);
    if (!json.isUndefined()) {
        each(json, [&](const JSValue& nodeJSON) {
            const auto uuid = nodeJSON.get("uuid");
            const auto type = nodeJSON.get("type");
            const auto value = invoke(self, "createNodeFromType")({type});
            nodes.set(uuid, value);
            auto created = nodes.get(uuid);
            created.set("uuid", uuid);
        });
        auto meta = JSValue::object();
        own(meta, "nodes", nodes);
        own(meta, "textures", self.get("textures"));
        each(json, [&](JSValue nodeJSON) {
            nodeJSON.set("meta", meta);
            const auto node = nodes.get(nodeJSON.get("uuid"));
            invoke(node, "deserialize")({nodeJSON});
            nodeJSON.erase("meta");
        });
    }
    return nodes;
}

// parse( json ) { ... }
JSValue parse(const JSValue& self, const Args& args, const Args&) {
    const auto json = arg(args, 0);
    auto node = invoke(self, "createNodeFromType")({json.get("type")});
    node.set("uuid", json.get("uuid"));
    const auto nodes = invoke(self, "parseNodes")({json.get("nodes")});
    auto meta = JSValue::object();
    own(meta, "nodes", nodes);
    own(meta, "textures", self.get("textures"));
    auto data = json;
    data.set("meta", meta);
    invoke(node, "deserialize")({data});
    data.erase("meta");
    return node;
}

// setTextures( value ) { this.textures = value; return this; }
JSValue setTextures(const JSValue& self, const Args& args, const Args&) {
    auto node = self;
    node.set("textures", arg(args, 0));
    return self;
}

// setNodes( value ) { this.nodes = value; return this; }
JSValue setNodes(const JSValue& self, const Args& args, const Args&) {
    auto node = self;
    node.set("nodes", arg(args, 0));
    return self;
}

// createNodeFromType( type ) { ... }
JSValue createNodeFromType(const JSValue& self, const Args& args, const Args&) {
    const auto type = arg(args, 0);
    if (self.get("nodes").get(type).isUndefined()) {
        ThreeUtils::error({JSValue("NodeLoader: Node type not found:"), type});
        return nativeTSLBaseBinding("float").call({}, {});
    }
    return self.get("nodes").get(type).construct({});
}
}  // namespace

JSValue nativeNodeLoaderModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeLoader module");
    nativeTSLBase();

    auto bindings = JSValue::object(nullptr);
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // super( manager ): the base class is the prototype of the constructor the holder carries.
            const auto baseCtor = captures[0].get("constructor").getPrototype();
            auto self = baseCtor.construct({arg(args, 0)}, target);
            self.set("textures", JSValue::object());
            self.set("nodes", JSValue::object());
            return self;
        },
        {holder}, "NodeLoader", 1);
    holder.set("constructor", ctor);
    ctor.setPrototype(Loader::prototype().get("constructor"));
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(Loader::prototype());
    prototype.set("constructor", ctor);
    ctor.setFunctionSourceText(NodeLoaderSource::classSource);

    for (const auto& [name, arity, callback, source] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {"load", 4, load, NodeLoaderSource::loadSource},
             {"parseNodes", 1, parseNodes, NodeLoaderSource::parseNodesSource},
             {"parse", 1, parse, NodeLoaderSource::parseSource},
             {"setTextures", 1, setTextures, NodeLoaderSource::setTexturesSource},
             {"setNodes", 1, setNodes, NodeLoaderSource::setNodesSource},
             {"createNodeFromType", 1, createNodeFromType, NodeLoaderSource::createNodeFromTypeSource}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(callback, {}, name, arity);
        descriptor.value->setFunctionSourceText(source);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeNodeLoaderBinding(const std::string& name) {
    const auto keeper = nativeNodeLoaderModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeNodeLoaderConstructor() {
    return nativeNodeLoaderBinding("default");
}
