#include "nodes/gpgpu/ComputeBuiltinNode.h"

#include "nodes/core/Node.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/gpgpu/GpgpuNodeSources.inc"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"

#include <iostream>
#include <initializer_list>
#include <tuple>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// The five module level built-in nodes live for the whole lifetime, so these are strong keepers.
JSValue& numWorkgroupsKeeper() { static thread_local JSValue value; return value; }
JSValue& workgroupIdKeeper() { static thread_local JSValue value; return value; }
JSValue& globalIdKeeper() { static thread_local JSValue value; return value; }
JSValue& localIdKeeper() { static thread_local JSValue value; return value; }
JSValue& subgroupSizeKeeper() { static thread_local JSValue value; return value; }

// isCacheable( /*builder*/ ) { return false; }
JSValue isCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// getHash( builder ) { return this.getBuiltinName( builder ); }
JSValue getHash(const JSValue& self, const Args& args, const Args&) {
    return invoke(self, "getBuiltinName")({arg(args, 0, nullptr)});
}

// generateNodeType( /*builder*/ ) { return this.nodeType; }
JSValue generateNodeType(const JSValue& self, const Args&, const Args&) {
    return self.get("nodeType");
}

// setBuiltinName( builtinName ) { this._builtinName = builtinName; return this; }
JSValue setBuiltinName(const JSValue& self, const Args& args, const Args&) {
    auto node = self;
    node.set("_builtinName", arg(args, 0, nullptr));
    return self;
}

// getBuiltinName( /*builder*/ ) { return this._builtinName; }
JSValue getBuiltinName(const JSValue& self, const Args&, const Args&) {
    return self.get("_builtinName");
}

// hasBuiltin( builder ) { return builder.hasBuiltin( this._builtinName ); }
JSValue hasBuiltin(const JSValue& self, const Args& args, const Args&) {
    return invoke(arg(args, 0, nullptr), "hasBuiltin")({self.get("_builtinName")})
        .toBoolean()
        ? JSValue(true)
        : JSValue(false);
}

// generate( builder, output ) { ... }
JSValue generate(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto output = arg(args, 1, nullptr); const auto builtinName = invoke(self, "getBuiltinName")({builder});
    const auto nodeType = invoke(self, "getNodeType")({builder});
    if (builder.get("shaderStage").strictEquals("compute")) {
        return invoke(builder, "format")({builtinName, nodeType, output});
    }
    ThreeUtils::warn({JSValue(std::string("TSL: Compute built-in value \"") +
                              builtinName.toString() + "\" can not be accessed in the " +
                              builder.get("shaderStage").toString() + " stage")});
    return invoke(builder, "generateConst")({nodeType});
}

// serialize( data ) { super.serialize( data ); data.global = this.global; data._builtinName = this._builtinName; }
JSValue serialize(const JSValue& self, const Args& args, const Args&) {
    const auto data = arg(args, 0, nullptr);
    const auto baseSerialize = nativeNodeConstructor().get("prototype").get("serialize");
    if (baseSerialize.isCallable()) baseSerialize.call(self, {data});
    auto target = data;
    target.set("global", self.get("global"));
    target.set("_builtinName", self.get("_builtinName"));
    return {};
}

// deserialize( data ) { super.deserialize( data ); this.global = data.global; this._builtinName = data._builtinName; }
JSValue deserialize(const JSValue& self, const Args& args, const Args&) {
    const auto data = arg(args, 0, nullptr);
    const auto baseDeserialize = nativeNodeConstructor().get("prototype").get("deserialize");
    if (baseDeserialize.isCallable()) baseDeserialize.call(self, {data});
    auto node = self;
    node.set("global", data.get("global"));
    node.set("_builtinName", data.get("_builtinName"));
    return {};
}
}  // namespace

JSValue nativeComputeBuiltinNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ComputeBuiltinNode module");

    auto bindings = JSValue::object(nullptr);
    const auto base = nativeNodeConstructor();
    const auto ctor = makeClass(
        "ComputeBuiltinNode", 2, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // super( nodeType ) — the second constructor parameter is the node type.
            auto self = superConstruct(target, captures, {arg(args, 1, nullptr)});
            self.set("_builtinName", arg(args, 0, nullptr));
            return self;
        },
        {}, ComputeBuiltinNodeSource::classSource, ComputeBuiltinNodeSource::typeSource);
    auto prototype = ctor.get("prototype");
    // The three hooks whose builder parameter is commented out in the source take no parameter at all.
    method(prototype, "isCacheable", 0, isCacheable, {}, ComputeBuiltinNodeSource::isCacheableSource);
    method(prototype, "getHash", 1, getHash, {}, ComputeBuiltinNodeSource::getHashSource);
    method(prototype, "generateNodeType", 0, generateNodeType, {},
           ComputeBuiltinNodeSource::generateNodeTypeSource);
    method(prototype, "setBuiltinName", 1, setBuiltinName, {},
           ComputeBuiltinNodeSource::setBuiltinNameSource);
    method(prototype, "getBuiltinName", 0, getBuiltinName, {},
           ComputeBuiltinNodeSource::getBuiltinNameSource);
    method(prototype, "hasBuiltin", 1, hasBuiltin, {}, ComputeBuiltinNodeSource::hasBuiltinSource);
    method(prototype, "generate", 2, generate, {}, ComputeBuiltinNodeSource::generateSource);
    method(prototype, "serialize", 1, serialize, {}, ComputeBuiltinNodeSource::serializeSource);
    method(prototype, "deserialize", 1, deserialize, {},
           ComputeBuiltinNodeSource::deserializeSource);

    const auto computeBuiltin = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            return captures[0].construct({arg(args, 0, nullptr), arg(args, 1, nullptr)});
        },
        {ctor}, "computeBuiltin", 2);

    auto makeBuiltin = [&](JSValue& keeper, const char* name, const char* nodeType) -> JSValue {
        if (!keeper.toBoolean()) {
            keeper = computeBuiltin.call({}, {JSValue(name), JSValue(nodeType)});
        }
        return keeper;
    };
    own(bindings, "default", ctor);
    own(bindings, "numWorkgroups", makeBuiltin(numWorkgroupsKeeper(), "numWorkgroups", "uvec3"));
    own(bindings, "workgroupId", makeBuiltin(workgroupIdKeeper(), "workgroupId", "uvec3"));
    own(bindings, "globalId", makeBuiltin(globalIdKeeper(), "globalId", "uvec3"));
    own(bindings, "localId", makeBuiltin(localIdKeeper(), "localId", "uvec3"));
    own(bindings, "subgroupSize", makeBuiltin(subgroupSizeKeeper(), "subgroupSize", "uint"));
    return publish(bindings, lexical(), weak);
}

JSValue nativeComputeBuiltinNodeBinding(const std::string& name) {
    const auto keeper = nativeComputeBuiltinNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeComputeBuiltinNodeConstructor() {
    return nativeComputeBuiltinNodeBinding("default");
}
