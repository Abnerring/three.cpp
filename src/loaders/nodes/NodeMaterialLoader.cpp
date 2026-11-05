#include "loaders/nodes/NodeMaterialLoader.h"

#include "loaders/MaterialLoader.h"
#include "loaders/nodes/NodeMaterialLoaderSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"

#include <string>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// parse( json ) { const material = super.parse( json ); const nodes = this.nodes; ... }
JSValue parse(const JSValue& self, const Args& args, const Args&) {
    const auto json = arg(args, 0, nullptr);
    const auto baseParse = MaterialLoader::prototype().get("parse");
    const auto material = baseParse.call(self, {json});
    auto created = material;
    const auto nodes = self.get("nodes");
    const auto inputNodes = json.get("inputNodes");
    if (inputNodes.toBoolean() && !inputNodes.isNull()) {
        // for ( const property in inputNodes ) — enumerable own properties here.
        for (const auto& property : inputNodes.ownPropertyNames()) {
            const auto descriptor = inputNodes.getOwnPropertyDescriptor(property);
            if (!descriptor || !descriptor->enumerable.value_or(false)) continue;
            const auto uuid = inputNodes.get(property.c_str());
            created.set(property.c_str(), nodes.get(uuid.toString()));
        }
    }
    return material;
}

// setNodes( value ) { this.nodes = value; return this; }
JSValue setNodes(const JSValue& self, const Args& args, const Args&) {
    auto loader = self;
    loader.set("nodes", arg(args, 0, nullptr));
    return self;
}

// setNodeMaterials( value ) { this.nodeMaterials = value; return this; }
JSValue setNodeMaterials(const JSValue& self, const Args& args, const Args&) {
    auto loader = self;
    loader.set("nodeMaterials", arg(args, 0, nullptr));
    return self;
}

// createMaterialFromType( type ) { const materialClass = this.nodeMaterials[ type ]; ... }
JSValue createMaterialFromType(const JSValue& self, const Args& args, const Args&) {
    const auto type = arg(args, 0, nullptr);
    const auto materialClass = self.get("nodeMaterials").get(type.toString());
    if (!materialClass.isUndefined()) {
        return materialClass.construct({});
    }
    const auto baseCreate = MaterialLoader::prototype().get("createMaterialFromType");
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
            auto self = base.construct({arg(args, 0, nullptr)}, target);
            self.set("nodes", JSValue::object(nullptr));
            self.set("nodeMaterials", JSValue::object(nullptr));
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
        descriptor.value = JSValue::function(callback, {}, name, arity);
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
