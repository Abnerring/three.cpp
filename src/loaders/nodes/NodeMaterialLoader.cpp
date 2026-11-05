#include "loaders/nodes/NodeMaterialLoader.h"

#include "loaders/MaterialLoader.h"
#include "loaders/nodes/NodeMaterialLoaderSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

#include <string>
#include <unordered_set>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// Snapshot keys once per prototype, while checking each descriptor at the time
// it is visited. Non-enumerable own properties still shadow inherited names.
class ForInCursor {
    JSValue object;
    std::vector<JSValue> keys;
    std::unordered_set<std::string> visited;
    std::size_t index = 0;
    bool loaded = false;
  public:
    explicit ForInCursor(const JSValue& value)
        : object(value.isNull() || value.isUndefined() ? JSValue(nullptr) : value.toObject()) {}
    std::optional<JSValue> next() {
        while (!object.isNull()) {
            if (!loaded) { keys = object.ownPropertyKeys(); index = 0; loaded = true; }
            while (index < keys.size()) {
                const auto key = keys[index++];
                if (key.isSymbol() || visited.contains(key.string())) continue;
                const auto descriptor = object.getOwnPropertyDescriptor(key);
                if (!descriptor) continue;
                visited.insert(key.string());
                if (descriptor->enumerable.value_or(false)) return key;
            }
            object = object.getPrototype(); loaded = false;
        }
        return std::nullopt;
    }
};

// parse( json ) { const material = super.parse( json ); const nodes = this.nodes; ... }
JSValue parse(const JSValue& self, const Args& args, const Args& captures) {
    const auto json = arg(args, 0);
    const auto baseParse = captures[0].getPrototype().get("parse", self);
    const auto material = baseParse.call(self, {json});
    auto created = material;
    const auto nodes = self.get("nodes");
    const auto inputNodes = json.get("inputNodes");
    ForInCursor cursor(inputNodes);
    while (const auto property = cursor.next()) {
        const auto uuid = inputNodes.get(*property);
        created.set(*property, nodes.get(uuid));
    }
    return material;
}

// setNodes( value ) { this.nodes = value; return this; }
JSValue setNodes(const JSValue& self, const Args& args, const Args&) {
    auto loader = self;
    loader.set("nodes", arg(args, 0));
    return self;
}

// setNodeMaterials( value ) { this.nodeMaterials = value; return this; }
JSValue setNodeMaterials(const JSValue& self, const Args& args, const Args&) {
    auto loader = self;
    loader.set("nodeMaterials", arg(args, 0));
    return self;
}

// createMaterialFromType( type ) { const materialClass = this.nodeMaterials[ type ]; ... }
JSValue createMaterialFromType(const JSValue& self, const Args& args, const Args& captures) {
    const auto type = arg(args, 0);
    const auto materialClass = self.get("nodeMaterials").get(type);
    if (!materialClass.isUndefined()) {
        return materialClass.construct({});
    }
    const auto baseCreate = captures[0].getPrototype().get("createMaterialFromType", self);
    return baseCreate.call(self, {type});
}
}  // namespace

JSValue nativeNodeMaterialLoaderModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "NodeMaterialLoader module");

    const auto baseCtor = MaterialLoader::prototype().get("constructor");
    auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // super( manager ): the base class is the prototype of the constructor the holder carries.
            const auto base = captures[0].get("constructor").getPrototype();
            auto self = base.construct({arg(args, 0)}, target);
            self.set("nodes", JSValue::object());
            self.set("nodeMaterials", JSValue::object());
            return self;
        },
        {holder}, "NodeMaterialLoader", 1);
    holder.set("constructor", ctor);
    ctor.setPrototype(baseCtor);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(MaterialLoader::prototype());
    prototype.set("constructor", ctor);
    ctor.setFunctionSourceText(NodeMaterialLoaderSource::classSource);

    for (const auto& [name, arity, callback, source] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction, const char*>>{
             {"parse", 1, parse, NodeMaterialLoaderSource::parseSource},
             {"setNodes", 1, setNodes, NodeMaterialLoaderSource::setNodesSource},
             {"setNodeMaterials", 1, setNodeMaterials,
              NodeMaterialLoaderSource::setNodeMaterialsSource},
             {"createMaterialFromType", 1, createMaterialFromType,
              NodeMaterialLoaderSource::createMaterialFromTypeSource}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(callback, {prototype}, name, arity);
        descriptor.value->setFunctionSourceText(source);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    return publish(bindings, lexical(), weak);
}

JSValue nativeNodeMaterialLoaderBinding(const std::string& name) {
    const auto keeper = nativeNodeMaterialLoaderModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeNodeMaterialLoaderConstructor() {
    return nativeNodeMaterialLoaderBinding("default");
}
