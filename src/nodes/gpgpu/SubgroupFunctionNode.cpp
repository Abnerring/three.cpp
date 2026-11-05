#include "nodes/gpgpu/SubgroupFunctionNode.h"

#include "nodes/core/Node.h"
#include "nodes/gpgpu/GpgpuNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"

#include <cstddef>
#include <initializer_list>
#include <utility>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// The static name constants, used by generateNodeType and generate.
constexpr const char* kSubgroupElect = "subgroupElect";
constexpr const char* kSubgroupBallot = "subgroupBallot";
constexpr const char* kSubgroupBroadcast = "subgroupBroadcast";
constexpr const char* kSubgroupShuffle = "subgroupShuffle";
constexpr const char* kQuadBroadcast = "quadBroadcast";
constexpr const char* kSubgroupShuffleXor = "subgroupShuffleXor";
constexpr const char* kSubgroupShuffleUp = "subgroupShuffleUp";
constexpr const char* kSubgroupShuffleDown = "subgroupShuffleDown";

// getInputType( builder ) { ... }
JSValue getInputType(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto aNode = self.get("aNode");
    const auto bNode = self.get("bNode");
    const auto aType = aNode.toBoolean() ? invoke(aNode, "getNodeType")({builder}) : JSValue(nullptr);
    const auto bType = bNode.toBoolean() ? invoke(bNode, "getNodeType")({builder}) : JSValue(nullptr);
    const auto aLen = invoke(builder, "isMatrix")({aType}).toBoolean()
        ? JSValue(0.0)
        : invoke(builder, "getTypeLength")({aType});
    const auto bLen = invoke(builder, "isMatrix")({bType}).toBoolean()
        ? JSValue(0.0)
        : invoke(builder, "getTypeLength")({bType});
    return aLen.toNumber() > bLen.toNumber() ? aType : bType;
}

// setup( builder ) { builder.enableSubGroups(); return super.setup( builder ); }
JSValue setup(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    invoke(builder, "enableSubGroups")({});
    const auto baseSetup = nativeNodeConstructor().get("prototype").get("setup");
    if (baseSetup.isCallable()) return baseSetup.call(self, {builder});
    return {};
}

// generateNodeType( builder ) { ... }
JSValue generateNodeType(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto method = self.get("method").toString();
    if (method == kSubgroupElect) return JSValue("bool");
    if (method == kSubgroupBallot) return JSValue("uvec4");
    return invoke(self, "getInputType")({builder});
}

// generate( builder, output ) { ... }
JSValue generate(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto output = arg(args, 1, nullptr);
    const auto methodValue = self.get("method");
    const auto method = methodValue.toString();
    if (builder.get("shaderStage").strictEquals("vertex")) {
        ThreeUtils::error({JSValue("TSL: \"" + method + "\" is not supported in the vertex shader stage.")});
    }
    const auto type = invoke(self, "getNodeType")({builder});
    const auto inputType = invoke(self, "getInputType")({builder});
    const auto a = self.get("aNode");
    const auto b = self.get("bNode");

    auto params = JSValue::array();
    std::size_t count = 0;
    const bool broadcastBranch = method == kSubgroupBroadcast || method == kSubgroupShuffle ||
        method == kQuadBroadcast;
    const bool shuffleOffsetBranch = method == kSubgroupShuffleXor ||
        method == kSubgroupShuffleDown || method == kSubgroupShuffleUp;
    if (broadcastBranch) {
        const auto bType = invoke(b, "getNodeType")({builder});
        params.set(count++, invoke(a, "build")({builder, type}));
        params.set(count++, invoke(b, "build")({builder, bType.strictEquals("float") ? JSValue("int") : type}));
    } else if (shuffleOffsetBranch) {
        params.set(count++, invoke(a, "build")({builder, type}));
        params.set(count++, invoke(b, "build")({builder, JSValue("uint")}));
    } else {
        if (!a.isNull()) params.set(count++, invoke(a, "build")({builder, inputType}));
        if (!b.isNull()) params.set(count++, invoke(b, "build")({builder, inputType}));
    }
    const auto paramsString = count == 0
        ? JSValue("()")
        : JSValue("( " + invoke(params, "join")({JSValue(", ")}).toString() + " )");
    return invoke(builder, "format")(
        {JSValue(invoke(builder, "getMethod")({methodValue, type}).toString() + paramsString.toString()),
         type, output});
}

// serialize( data ) { super.serialize( data ); data.method = this.method; }
JSValue serialize(const JSValue& self, const Args& args, const Args&) {
    const auto data = arg(args, 0, nullptr);
    const auto baseSerialize = nativeNodeConstructor().get("prototype").get("serialize");
    if (baseSerialize.isCallable()) baseSerialize.call(self, {data});
    auto target = data;
    target.set("method", self.get("method"));
    return {};
}

// deserialize( data ) { super.deserialize( data ); this.method = data.method; }
JSValue deserialize(const JSValue& self, const Args& args, const Args&) {
    const auto data = arg(args, 0, nullptr);
    const auto baseDeserialize = nativeNodeConstructor().get("prototype").get("deserialize");
    if (baseDeserialize.isCallable()) baseDeserialize.call(self, {data});
    auto node = self;
    node.set("method", data.get("method"));
    return {};
}
}  // namespace

JSValue nativeSubgroupFunctionNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "SubgroupFunctionNode module");
    nativeTSLCore();

    auto bindings = JSValue::object(nullptr);
    const auto base = nativeNodeConstructor();
    auto ctor = makeClass(
        "SubgroupFunctionNode", 1, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // super() with no node type.
            auto self = superConstruct(target, captures, {});
            self.set("method", arg(args, 0, nullptr));
            self.set("aNode", arg(args, 1, nullptr));
            self.set("bNode", arg(args, 2, nullptr));
            return self;
        },
        {}, SubgroupFunctionNodeSource::classSource, SubgroupFunctionNodeSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "getInputType", 1, getInputType, {}, SubgroupFunctionNodeSource::getInputTypeSource);
    method(prototype, "setup", 1, setup, {}, SubgroupFunctionNodeSource::setupSource);
    method(prototype, "generateNodeType", 1, generateNodeType, {},
           SubgroupFunctionNodeSource::generateNodeTypeSource);
    method(prototype, "generate", 2, generate, {}, SubgroupFunctionNodeSource::generateSource);
    method(prototype, "serialize", 1, serialize, {}, SubgroupFunctionNodeSource::serializeSource);
    method(prototype, "deserialize", 1, deserialize, {}, SubgroupFunctionNodeSource::deserializeSource);

    // The 25 static method name constants are static getters in the source.
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, const char*>>{
             {"SUBGROUP_ELECT", "subgroupElect"}, {"SUBGROUP_BALLOT", "subgroupBallot"},
             {"SUBGROUP_ADD", "subgroupAdd"},
             {"SUBGROUP_INCLUSIVE_ADD", "subgroupInclusiveAdd"},
             {"SUBGROUP_EXCLUSIVE_ADD", "subgroupExclusiveAdd"},
             {"SUBGROUP_MUL", "subgroupMul"},
             {"SUBGROUP_INCLUSIVE_MUL", "subgroupInclusiveMul"},
             {"SUBGROUP_EXCLUSIVE_MUL", "subgroupExclusiveMul"},
             {"SUBGROUP_AND", "subgroupAnd"}, {"SUBGROUP_OR", "subgroupOr"},
             {"SUBGROUP_XOR", "subgroupXor"}, {"SUBGROUP_MIN", "subgroupMin"},
             {"SUBGROUP_MAX", "subgroupMax"}, {"SUBGROUP_ALL", "subgroupAll"},
             {"SUBGROUP_ANY", "subgroupAny"},
             {"SUBGROUP_BROADCAST_FIRST", "subgroupBroadcastFirst"},
             {"QUAD_SWAP_X", "quadSwapX"}, {"QUAD_SWAP_Y", "quadSwapY"},
             {"QUAD_SWAP_DIAGONAL", "quadSwapDiagonal"},
             {"SUBGROUP_BROADCAST", "subgroupBroadcast"},
             {"SUBGROUP_SHUFFLE", "subgroupShuffle"},
             {"SUBGROUP_SHUFFLE_XOR", "subgroupShuffleXor"},
             {"SUBGROUP_SHUFFLE_UP", "subgroupShuffleUp"},
             {"SUBGROUP_SHUFFLE_DOWN", "subgroupShuffleDown"},
             {"QUAD_BROADCAST", "quadBroadcast"}}) {
        JSPropertyDescriptor descriptor;
        descriptor.configurable = true;
        descriptor.getter = JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue { return captures[0]; },
            {JSValue(value)}, name, 0);
        ctor.defineProperty(name, descriptor);
    }

    // const subgroupElect = nodeProxyIntent( SubgroupFunctionNode, SubgroupFunctionNode.SUBGROUP_ELECT ).setParameterLength( 0 )
    auto makeFactory = [&](const char* constant, unsigned arity) -> JSValue {
        const auto proxy = invoke(nativeTSLCoreBinding("nodeProxyIntent"), "call")(
            {ctor, JSValue(constant)});
        return invoke(proxy, "setParameterLength")({JSValue(static_cast<double>(arity))});
    };

    own(bindings, "default", ctor);
    for (const auto& [name, constant, arity] : std::initializer_list<
             std::tuple<const char*, const char*, unsigned>>{
             {"subgroupElect", "subgroupElect", 0},
             {"subgroupBallot", "subgroupBallot", 1},
             {"subgroupAdd", "subgroupAdd", 1},
             {"subgroupInclusiveAdd", "subgroupInclusiveAdd", 1},
             {"subgroupExclusiveAdd", "subgroupExclusiveAdd", 1},
             {"subgroupMul", "subgroupMul", 1},
             {"subgroupInclusiveMul", "subgroupInclusiveMul", 1},
             {"subgroupExclusiveMul", "subgroupExclusiveMul", 1},
             {"subgroupAnd", "subgroupAnd", 1},
             {"subgroupOr", "subgroupOr", 1},
             {"subgroupXor", "subgroupXor", 1},
             {"subgroupMin", "subgroupMin", 1},
             {"subgroupMax", "subgroupMax", 1},
             {"subgroupAll", "subgroupAll", 1},
             {"subgroupAny", "subgroupAny", 1},
             {"subgroupBroadcastFirst", "subgroupBroadcastFirst", 1},
             {"quadSwapX", "quadSwapX", 1},
             {"quadSwapY", "quadSwapY", 1},
             {"quadSwapDiagonal", "quadSwapDiagonal", 1},
             {"subgroupBroadcast", "subgroupBroadcast", 2},
             {"subgroupShuffle", "subgroupShuffle", 2},
             {"subgroupShuffleXor", "subgroupShuffleXor", 2},
             {"subgroupShuffleUp", "subgroupShuffleUp", 2},
             {"subgroupShuffleDown", "subgroupShuffleDown", 2},
             {"quadBroadcast", "quadBroadcast", 1}}) {
        own(bindings, name, makeFactory(constant, arity));
    }
    return publish(bindings, lexical(), weak);
}

JSValue nativeSubgroupFunctionNodeBinding(const std::string& name) {
    const auto keeper = nativeSubgroupFunctionNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeSubgroupFunctionNodeConstructor() {
    return nativeSubgroupFunctionNodeBinding("default");
}
