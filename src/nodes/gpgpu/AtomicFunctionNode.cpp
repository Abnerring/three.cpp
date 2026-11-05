#include "nodes/gpgpu/AtomicFunctionNode.h"

#include "nodes/core/Node.h"
#include "nodes/core/StackNode.h"
#include "nodes/gpgpu/GpgpuNodeSources.inc"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"

#include <cstddef>
#include <initializer_list>
#include <tuple>

namespace {
using namespace LightingNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

// isCacheable( /*builder*/ ) { return false; }
JSValue isCacheable(const JSValue&, const Args&, const Args&) { return JSValue(false); }

// getInputType( builder ) { return this.pointerNode.getNodeType( builder ); }
JSValue getInputType(const JSValue& self, const Args& args, const Args&) {
    return invoke(self.get("pointerNode"), "getNodeType")({arg(args, 0, nullptr)});
}

// generateNodeType( builder ) { return this.getInputType( builder ); }
JSValue generateNodeType(const JSValue& self, const Args& args, const Args&) {
    return invoke(self, "getInputType")({arg(args, 0, nullptr)});
}

// generate( builder ) { ... }
JSValue generate(const JSValue& self, const Args& args, const Args&) {
    const auto builder = arg(args, 0, nullptr);
    const auto method = self.get("method");
    if (builder.get("shaderStage").strictEquals("vertex")) {
        ThreeUtils::error(
            {JSValue("TSL: \"" + method.toString() + "\" is not supported in the vertex stage.")});
    }
    const auto properties = invoke(builder, "getNodeProperties")({self});
    const auto parents = properties.get("parents");
    const auto type = invoke(self, "getNodeType")({builder});
    const auto inputType = invoke(self, "getInputType")({builder});
    const auto a = self.get("pointerNode");
    const auto b = self.get("valueNode");
    // const params = [];  params.push( `&${ a.build( builder, inputType ) }` );  if ( b !== null ) ...
    auto params = JSValue::array();
    params.set(static_cast<std::size_t>(0),
               JSValue("&" + invoke(a, "build")({builder, inputType}).toString()));
    if (!b.isNull()) {
        params.set(static_cast<std::size_t>(1), invoke(b, "build")({builder, inputType}));
    }
    const auto joined = invoke(params, "join")({JSValue(", ")}).toString();
    const auto methodSnippet =
        invoke(builder, "getMethod")({method, type}).toString() + "( " + joined + " )";

    bool isVoid = false;
    if (parents.toBoolean()) {
        isVoid = parents.length() == 1 &&
            parents.get(static_cast<std::size_t>(0)).get("isStackNode").strictEquals(true);
    }
    if (isVoid) {
        invoke(builder, "addLineFlowCode")({JSValue(methodSnippet), self});
        return {};
    }
    const auto nodeVar = invoke(builder, "getVarFromNode")(
        {self, JSValue(nullptr), type, JSValue(nullptr), JSValue(true), JSValue(true)});
    const auto propertyName = invoke(builder, "getPropertyName")({nodeVar});
    const auto statement = invoke(builder, "generateLetStatement")({type, propertyName}).toString() +
        " = " + methodSnippet;
    invoke(builder, "addLineFlowCode")({JSValue(statement), self});
    return propertyName;
}
}  // namespace

JSValue nativeAtomicFunctionNodeModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "AtomicFunctionNode module");
    nativeStackNodeModule();
    nativeTSLBase();
    nativeTSLCore();

    auto bindings = JSValue::object(nullptr);
    const auto base = nativeNodeConstructor();
    auto ctor = makeClass(
        "AtomicFunctionNode", 3, base,
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            // super( 'uint' )
            auto self = superConstruct(target, captures, {JSValue("uint")});
            self.set("method", arg(args, 0, nullptr));
            self.set("pointerNode", arg(args, 1, nullptr));
            self.set("valueNode", arg(args, 2, nullptr));
            self.set("parents", true);
            return self;
        },
        {}, AtomicFunctionNodeSource::classSource, AtomicFunctionNodeSource::typeSource);
    auto prototype = ctor.get("prototype");
    method(prototype, "isCacheable", 0, isCacheable, {}, AtomicFunctionNodeSource::isCacheableSource);
    method(prototype, "getInputType", 1, getInputType, {}, AtomicFunctionNodeSource::getInputTypeSource);
    method(prototype, "generateNodeType", 1, generateNodeType, {},
           AtomicFunctionNodeSource::generateNodeTypeSource);
    method(prototype, "generate", 1, generate, {}, AtomicFunctionNodeSource::generateSource);

    // The nine static method name constants are static getters in the source.
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, const char*>>{
             {"ATOMIC_LOAD", "atomicLoad"}, {"ATOMIC_STORE", "atomicStore"},
             {"ATOMIC_ADD", "atomicAdd"}, {"ATOMIC_SUB", "atomicSub"},
             {"ATOMIC_MAX", "atomicMax"}, {"ATOMIC_MIN", "atomicMin"},
             {"ATOMIC_AND", "atomicAnd"}, {"ATOMIC_OR", "atomicOr"},
             {"ATOMIC_XOR", "atomicXor"}}) {
        JSPropertyDescriptor descriptor;
        descriptor.configurable = true;
        descriptor.getter = JSValue::function(
            +[](const JSValue&, const Args&, const Args& captures) -> JSValue { return captures[0]; },
            {JSValue(value)}, name, 0);
        ctor.defineProperty(name, descriptor);
    }

    // const atomicNode = nodeProxy( AtomicFunctionNode );
    const auto atomicNode = nativeTSLBaseBinding("nodeProxy").call({}, {ctor});

    // export const atomicFunc = ( method, pointerNode, valueNode ) => atomicNode( ... ).toStack();
    const auto atomicFunc = JSValue::function(
        +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto node = captures[0].call(
                {}, {arg(args, 0, nullptr), arg(args, 1, nullptr), arg(args, 2, nullptr)});
            return invoke(node, "toStack")({});
        },
        {atomicNode}, "atomicFunc", 3);

    auto makeAtomic = [&](const char* name, const char* method, unsigned arity,
                          bool withValue) -> JSValue {
        return JSValue::function(
            +[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
                const auto node = captures[0].call(
                    {}, {captures[1], arg(args, 0, nullptr),
                         captures[2].strictEquals(true) ? arg(args, 1, nullptr) : JSValue(nullptr)});
                return invoke(node, "toStack")({});
            },
            {atomicNode, JSValue(method), JSValue(withValue)}, name, arity);
    };

    own(bindings, "default", ctor);
    own(bindings, "atomicFunc", atomicFunc);
    own(bindings, "atomicLoad", makeAtomic("atomicLoad", "atomicLoad", 1, false));
    own(bindings, "atomicStore", makeAtomic("atomicStore", "atomicStore", 2, true));
    own(bindings, "atomicAdd", makeAtomic("atomicAdd", "atomicAdd", 2, true));
    own(bindings, "atomicSub", makeAtomic("atomicSub", "atomicSub", 2, true));
    own(bindings, "atomicMax", makeAtomic("atomicMax", "atomicMax", 2, true));
    own(bindings, "atomicMin", makeAtomic("atomicMin", "atomicMin", 2, true));
    own(bindings, "atomicAnd", makeAtomic("atomicAnd", "atomicAnd", 2, true));
    own(bindings, "atomicOr", makeAtomic("atomicOr", "atomicOr", 2, true));
    own(bindings, "atomicXor", makeAtomic("atomicXor", "atomicXor", 2, true));
    return publish(bindings, lexical(), weak);
}

JSValue nativeAtomicFunctionNodeBinding(const std::string& name) {
    const auto keeper = nativeAtomicFunctionNodeModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeAtomicFunctionNodeConstructor() {
    return nativeAtomicFunctionNodeBinding("default");
}
