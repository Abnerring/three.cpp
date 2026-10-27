#include "nodes/tsl/TSLShaderNodes.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/StackNode.h"
#include "nodes/core/StackTrace.h"
#include "core/JSIterator.h"
#include "core/JSProxy.h"
#include "core/JSPromise.h"
#include "core/JSSet.h"
#include "utils.h"
#include <unordered_set>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index, JSValue fallback = {}) {
    return index < args.size() && !args[index].isUndefined() ? args[index] : fallback;
}
struct Invocation {
    JSValue receiver, method;
    JSValue operator()(const Args& args = {}) const { return method.call(receiver, args); }
};
Invocation invoke(const JSValue& receiver, const char* name) { return {receiver, receiver.get(name)}; }
void own(JSValue object, const JSValue& key, const JSValue& value) {
    JSPropertyDescriptor descriptor; descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue literal(std::initializer_list<std::pair<const char*, JSValue>> entries = {}) {
    auto result = JSValue::object(); for (const auto& [key, value] : entries) own(result, key, value); return result;
}
JSValue array(const Args& entries = {}) {
    auto result = JSValue::array();
    for (const auto& value : entries) own(result, std::to_string(result.length()), value);
    return result;
}
template<class F> void each(const JSValue& iterable, F body) {
    auto iterator = getJSIterator(iterable);
    while (const auto item = stepJSIterator(iterator)) {
        try { body(*item); } catch (...) { closeJSIterator(iterator, std::current_exception()); }
    }
}
template<class F> void forIn(const JSValue& value, F body) {
    if (value.isNull() || value.isUndefined()) return;
    if (!value.isObject()) {
        if (value.type() == JSValue::Type::String)
            for (double i = 0; i < value.get("length").toNumber(); ++i) body(JSValue(i).toString());
        return;
    }
    std::unordered_set<std::string> visited;
    std::vector<std::string> names;
    for (auto level = value; !level.isNull(); level = level.getPrototype()) {
        for (const auto& name : level.ownPropertyNames()) {
            if (visited.contains(name)) continue;
            const auto descriptor = level.getOwnPropertyDescriptor(name);
            if (!descriptor) continue;
            visited.insert(name);
            if (descriptor->enumerable.value_or(false)) names.push_back(name);
        }
    }
    for (const auto& name : names) if (value.has(name)) body(name);
}
bool instanceOfNode(const JSValue& value, const JSValue& constructor) {
    const auto hasInstance = constructor.get(JSValue::wellKnownSymbol("hasInstance"));
    if (!hasInstance.isUndefined() && !hasInstance.isNull()) return hasInstance.call(constructor, {value}).toBoolean();
    if (!constructor.isCallable()) throw JSTypeError("Right-hand side of instanceof is not callable");
    if (!value.isObject()) return false;
    const auto prototype = constructor.get("prototype");
    if (!prototype.isObject()) throw JSTypeError("Function has non-object prototype in instanceof check");
    for (auto current = value.getPrototype(); !current.isNull(); current = current.getPrototype())
        if (current.sameValue(prototype)) return true;
    return false;
}
JSValue getLayoutParameters(const JSValue& params, const JSValue& helpers) {
    helpers.get("nodeObjects").call({}, {params});
    if (helpers.get("isArrayAsParameter").call({}, {params}).toBoolean()) {
        auto output = JSValue::array();
        each(params, [&](const JSValue& input) { own(output, std::to_string(output.length()), input); });
        return output;
    }
    return params.get("0");
}

// Resumable generator bodies retain their lexical iterable as a managed edge.
constexpr const char* generatorBrand = "TSL parameter Generator";
void finishGenerator(JSValue state) {
    state.set("done", true); state.set("executing", false);
    state.set("iterator", {}); state.set("next", {});
}
JSValue resumeGenerator(const JSValue& receiver, const Args& args, const Args& captures) {
    if (!receiver.isHostObject(generatorBrand)) throw JSTypeError("Incompatible Generator receiver");
    auto state = receiver.getHostSlot("state");
    if (state.get("executing").toBoolean()) throw JSTypeError("Generator is already running");
    const bool throwing = captures[0].strictEquals("throw");
    if (!captures[0].strictEquals("next")) {
        state.set("executing", true);
        try {
            if (!state.get("done").toBoolean() && state.get("started").toBoolean() && !state.get("single").toBoolean()) {
                JSIteratorRecord record{state.get("iterator"), state.get("next"), false};
                if (throwing) closeJSIterator(record, std::make_exception_ptr(JSThrownValue(arg(args, 0))));
                closeJSIterator(record);
            }
            finishGenerator(state);
            if (throwing) throw JSThrownValue(arg(args, 0));
            return literal({{"value", arg(args, 0)}, {"done", true}});
        } catch (...) { finishGenerator(state); throw; }
    }
    if (state.get("done").toBoolean()) return literal({{"value", {}}, {"done", true}});
    state.set("executing", true);
    try {
        if (state.get("single").toBoolean()) {
            if (state.get("started").toBoolean()) { finishGenerator(state); return literal({{"value", {}}, {"done", true}}); }
            state.set("started", true); state.set("executing", false); return literal({{"value", {}}, {"done", false}});
        }
        if (!state.get("started").toBoolean()) {
            state.set("started", true);
            const auto record = getJSIterator(state.get("iterable"));
            state.set("iterator", record.iterator); state.set("next", record.nextMethod);
        }
        JSIteratorRecord record{state.get("iterator"), state.get("next"), false};
        const auto value = stepJSIterator(record);
        if (!value) { finishGenerator(state); return literal({{"value", {}}, {"done", true}}); }
        state.set("executing", false); return literal({{"value", *value}, {"done", false}});
    } catch (...) { finishGenerator(state); throw; }
}
JSValue generatorPrototype() {
    static thread_local JSWeakValue weak;
    if (const auto existing = weak.lock(); existing && existing->isObject()) return *existing;
    auto constructor = JSValue::callableBuiltinConstructor([](const JSValue&, const JSValue&, const Args&, const Args&) -> JSValue {
        throw std::logic_error("Dynamic generator source is not supported by the native C++ runtime");
    }, {}, "GeneratorFunction", 1);
    auto functionPrototype = constructor.get("prototype");
    const auto functionBase = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }).getPrototype();
    functionPrototype.setPrototype(functionBase); constructor.setPrototype(functionBase.get("constructor"));
    auto prototype = JSValue::object(nativeIteratorPrototype());
    const auto define = [](JSValue object, const JSValue& key, const JSValue& value, bool writable) {
        JSPropertyDescriptor descriptor; descriptor.value = value; descriptor.writable = writable; descriptor.configurable = true; object.defineProperty(key, descriptor);
    };
    define(functionPrototype, "prototype", prototype, false); define(functionPrototype, "constructor", constructor, false);
    define(functionPrototype, JSValue::wellKnownSymbol("toStringTag"), "GeneratorFunction", false);
    define(prototype, "constructor", functionPrototype, false);
    for (const auto* name : {"next", "return", "throw"}) define(prototype, name, JSValue::function(resumeGenerator, {name}, name, 1), true);
    define(prototype, JSValue::wellKnownSymbol("toStringTag"), "Generator", false);
    prototype.retainForHeapLifetime(); weak = JSWeakValue(prototype); return prototype;
}
JSValue makeGeneratorFunction(const JSValue& iterable, bool singleUndefined) {
    auto holder = JSValue::object(nullptr);
    auto function = JSValue::function([](const JSValue&, const Args&, const Args& captures) {
        auto state = literal({{"iterable", captures[0]}, {"started", false}, {"done", false}, {"executing", false}, {"single", captures[1]}});
        const auto token = std::make_shared<unsigned>(0);
        auto iterator = JSValue::hostObject(token, token.get(), generatorBrand);
        const auto prototype = captures[2].get("function").get("prototype");
        iterator.setPrototype(prototype.isObject() ? prototype : generatorPrototype()); iterator.setHostSlot("state", state); return iterator;
    }, {iterable, singleUndefined, holder}, "value", 0);
    function.setPrototype(generatorPrototype().get("constructor"));
    JSPropertyDescriptor instancePrototype; instancePrototype.value = JSValue::object(generatorPrototype()); instancePrototype.writable = true;
    function.defineProperty("prototype", instancePrototype); holder.set("function", function); return function;
}
JSValue getProxyParameters(const JSValue& params, const JSValue& helpers) {
    auto state = literal({{"index", 0}});
    helpers.get("nodeObjects").call({}, {params});
    auto handler = literal();
    own(handler, "get", JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue {
        const auto target = arg(args, 0), property = arg(args, 1), receiver = arg(args, 2);
        const auto params = captures[0], helpers = captures[2]; auto state = captures[1];
        if (property.strictEquals("length")) return params.get("length");
        if (property.sameValue(JSValue::wellKnownSymbol("iterator")))
            return makeGeneratorFunction(params, false);
        JSValue value;
        if (params.get("length").toNumber() > 0) {
            // Object.getPrototypeOf is deliberately a live intrinsic property.
            const auto objectCtor = JSValue::intrinsicObjectConstructor();
            const auto prototype = objectCtor.get("getPrototypeOf").call(objectCtor, {params.get("0")});
            if (prototype.sameValue(objectCtor.get("prototype"))) {
                const auto objectTarget = params.get("0");
                if (objectTarget.get(property).isUndefined()) {
                    const auto index = state.get("index").toNumber(); state.set("index", index + 1);
                    value = objectTarget.get(JSValue(index));
                } else value = objectTarget.get(property, receiver);
            } else if (instanceOfNode(params.get("0"), captures[3])) {
                if (params.get(property).isUndefined()) {
                    const auto index = state.get("index").toNumber(); state.set("index", index + 1);
                    value = params.get(JSValue(index));
                } else value = params.get(property, receiver);
            }
        } else value = target.get(property, receiver);
        return helpers.get("nodeObject").call({}, {value});
    }, {params, state, helpers, nativeNodeConstructor()}, "get", 3));
    return createNativeProxy(params, handler);
}
JSValue secureBuilder(const JSValue& builder) {
    auto handler = literal();
    own(handler, "get", JSValue::function([](const JSValue&, const Args& args, const Args&) -> JSValue {
        const auto target = arg(args, 0), property = arg(args, 1), receiver = arg(args, 2);
        if (property.sameValue(JSValue::wellKnownSymbol("iterator")))
            return makeGeneratorFunction({}, true);
        return target.get(property, receiver);
    }, {}, "get", 3));
    return createNativeProxy(builder, handler);
}

JSValue shaderCallMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto method = captures[0].string(); const auto builder = arg(args, 0); const auto helpers = captures[1];
    if (method == "isCacheable") return invoke(self, "getOutputNode")({builder}).get("nodes").get("length").strictEquals(0);
    if (method == "generateNodeType") {
        const auto type = self.get("shaderNode").get("nodeType");
        return type.toBoolean() ? type : invoke(invoke(self, "getOutputNode")({builder}), "getNodeType")({builder});
    }
    if (method == "getElementType") return invoke(invoke(self, "getOutputNode")({builder}), "getElementType")({builder});
    if (method == "getMemberType") return invoke(invoke(self, "getOutputNode")({builder}), "getMemberType")({builder, arg(args, 1)});
    if (method == "call") {
        const auto shaderNode = self.get("shaderNode"), rawInputs = self.get("rawInputs");
        auto properties = invoke(builder, "getNodeProperties")({shaderNode});
        auto subBuild = invoke(builder, "getClosestSubBuild")({shaderNode.get("subBuilds")});
        if (!subBuild.toBoolean()) subBuild = "";
        const auto subBuildProperty = subBuild.toBoolean() ? subBuild : JSValue("default");
        if (properties.get(subBuildProperty).toBoolean()) return properties.get(subBuildProperty);
        const auto previousSubBuildFn = builder.get("subBuildFn"), previousFnCall = builder.get("fnCall");
        auto mutableBuilder = builder; mutableBuilder.set("subBuildFn", subBuild); mutableBuilder.set("fnCall", self);
        JSValue result = nullptr;
        if (shaderNode.get("layout").toBoolean()) {
            if (rawInputs.toBoolean()) {
                const auto inputs = shaderNode.get("layout").get("inputs");
                if (helpers.get("isArrayAsParameter").call({}, {rawInputs}).toBoolean()) {
                    for (double i = 0; i < inputs.get("length").toNumber(); ++i) {
                        const auto parameter = rawInputs.get(JSValue(i));
                        if (parameter.toBoolean() && parameter.get("isNode").toBoolean()) invoke(parameter, "build")({builder});
                    }
                } else {
                    const auto rawObject = rawInputs.get("0");
                    each(inputs, [&](const JSValue& input) {
                        const auto parameter = rawObject.get(input.get("name"));
                        if (parameter.toBoolean() && parameter.get("isNode").toBoolean()) invoke(parameter, "build")({builder});
                    });
                }
            }
            const auto functionNode = invoke(builder, "buildFunctionNode")({shaderNode});
            invoke(builder, "addInclude")({functionNode});
            const auto inputs = rawInputs.toBoolean() ? getLayoutParameters(rawInputs, helpers) : JSValue(nullptr);
            result = invoke(functionNode, "call")({inputs});
        } else {
            const auto secure = secureBuilder(builder);
            const auto inputs = rawInputs.toBoolean() ? getProxyParameters(rawInputs, helpers) : JSValue(nullptr);
            const bool hasParameters = isNativeArray(rawInputs) ? rawInputs.get("length").toNumber() > 0 : !rawInputs.isNull();
            const auto jsFunc = shaderNode.get("jsFunc");
            const auto output = hasParameters || jsFunc.get("length").toNumber() > 1 ? jsFunc.call({}, {inputs, secure}) : jsFunc.call({}, {secure});
            result = helpers.get("nodeObject").call({}, {output});
        }
        mutableBuilder.set("subBuildFn", previousSubBuildFn); mutableBuilder.set("fnCall", previousFnCall);
        if (shaderNode.get("once").toBoolean()) properties.set(subBuildProperty, result);
        return result;
    }
    if (method == "setupOutput") {
        invoke(builder, "addStack")();
        const auto target = builder.get("stack"); const auto result = invoke(self, "call")({builder});
        auto stack = target; stack.set("outputNode", result); return invoke(builder, "removeStack")();
    }
    if (method == "getOutputNode") {
        auto properties = invoke(builder, "getNodeProperties")({self});
        const auto subBuildOutput = invoke(builder, "getSubBuildOutput")({self});
        const auto assignmentKey = subBuildOutput.toPropertyKey();
        const auto existing = properties.get(subBuildOutput);
        const auto value = existing.toBoolean() ? existing : invoke(self, "setupOutput")({builder});
        properties.set(assignmentKey, value);
        const auto output = properties.get(subBuildOutput); const auto subBuild = invoke(builder, "getClosestSubBuild")({self});
        auto node = output; node.set("subBuild", subBuild); return properties.get(subBuildOutput);
    }
    if (method == "build") {
        JSValue result = nullptr;
        const auto buildStage = invoke(builder, "getBuildStage")();
        auto properties = invoke(builder, "getNodeProperties")({self});
        const auto subBuildOutput = invoke(builder, "getSubBuildOutput")({self});
        const auto outputNode = invoke(self, "getOutputNode")({builder});
        const auto previousFnCall = builder.get("fnCall"); auto mutableBuilder = builder; mutableBuilder.set("fnCall", self);
        if (buildStage.strictEquals("setup")) {
            const auto initialized = invoke(builder, "getSubBuildProperty")({"initialized", self});
            if (!properties.get(initialized).strictEquals(true)) {
                properties.set(initialized, true);
                const auto assignmentKey = subBuildOutput.toPropertyKey(); const auto value = invoke(self, "getOutputNode")({builder});
                properties.set(assignmentKey, value);
                invoke(properties.get(subBuildOutput), "build")({builder});
                if (self.get("shaderNode").get("subBuilds").toBoolean()) {
                    each(builder.get("chaining"), [&](const JSValue& node) {
                        auto data = invoke(builder, "getDataFromNode")({node, "any"});
                        const auto subBuilds = data.get("subBuilds"); data.set("subBuilds", subBuilds.toBoolean() ? subBuilds : createNativeSet());
                        each(self.get("shaderNode").get("subBuilds"), [&](const JSValue& subBuild) { invoke(data.get("subBuilds"), "add")({subBuild}); });
                    });
                }
            }
            result = properties.get(subBuildOutput);
        } else if (buildStage.strictEquals("analyze")) invoke(outputNode, "build")({builder, arg(args, 1, nullptr)});
        else if (buildStage.strictEquals("generate")) { result = invoke(outputNode, "build")({builder, arg(args, 1, nullptr)}); if (!result.toBoolean()) result = ""; }
        mutableBuilder.set("fnCall", previousFnCall); return result;
    }
    throw std::logic_error("Unknown ShaderCallNodeInternal method");
}

JSValue shaderMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto& name = captures[0].string();
    if (name == "isCacheable") return false;
    if (name == "setLayout") { self.set("layout", arg(args, 0)); return self; }
    if (name == "getLayout") return self.get("layout");
    if (name == "call") return captures[1].construct({self, arg(args, 0, nullptr)});
    if (name == "setup") return invoke(self, "call")();
    throw std::logic_error("Unknown ShaderNodeInternal method");
}
JSValue fnMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    if (name == "isCacheable") return false;
    if (name == "setLayout") {
        auto layout = arg(args, 0); const auto nodeType = self.get("shaderNode").get("nodeType");
        const auto layoutInputs = layout.get("inputs");
        if (!layoutInputs.isNull() && (!layoutInputs.isObject() || layoutInputs.isCallable())) {
            auto state = captures[1]; const auto id = state.get("fnId").toNumber(); state.set("fnId", id + 1);
            auto full = literal({{"name", "fn" + JSValue(id).toString()}, {"type", nodeType}, {"inputs", JSValue::array()}});
            forIn(layout, [&](const std::string& name) {
                if (name != "return") {
                    const auto push = invoke(full.get("inputs"), "push");
                    const auto input = literal({{"name", name}, {"type", layout.get(name)}}); push({input});
                }
            });
            layout = full;
        }
        invoke(self.get("shaderNode"), "setLayout")({layout}); return self;
    }
    if (name == "generateNodeType") {
        const auto type = invoke(self.get("shaderNode"), "getNodeType")({arg(args, 0)}); return type.toBoolean() ? type : JSValue("float");
    }
    if (name == "call") {
        const auto result = invoke(self.get("shaderNode"), "call")({array(args)});
        if (self.get("shaderNode").get("nodeType").strictEquals("void")) invoke(result, "toStack")();
        return invoke(result, "toVarIntent")();
    }
    if (name == "once") { auto shader = self.get("shaderNode"); shader.set("once", true); shader = self.get("shaderNode"); shader.set("subBuilds", arg(args, 0, nullptr)); return self; }
    if (name == "generate") {
        const auto type = invoke(self, "getNodeType")({arg(args, 0)});
        ThreeUtils::error({"TSL: \"Fn()\" was declared but not invoked. Try calling it like \"Fn()( ...params )\".", self.get("stackTrace")});
        return invoke(arg(args, 0), "generateConst")({type});
    }
    throw std::logic_error("Unknown FnNode method");
}
void installMethods(JSValue prototype, JSValue::NativeFunction callback, const JSValue& state,
                    std::initializer_list<std::pair<const char*, unsigned>> methods) {
    for (const auto& [name, arity] : methods) {
        JSPropertyDescriptor descriptor; descriptor.value = JSValue::function(callback, {name, state}, name, arity);
        descriptor.writable = descriptor.configurable = true; prototype.defineProperty(name, descriptor);
    }
}
}

void installNativeTSLShaderExports(const JSValue& module) {
    const auto base = nativeNodeConstructor();
    auto helpers = literal({{"nodeObject", module.get("nodeObject")}, {"nodeObjects", module.get("nodeObjects")},
                            {"isArrayAsParameter", nativeNodeUtils().get("isArrayAsParameter")}});
    auto callHolder = JSValue::object(nullptr);
    auto callConstructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) {
        auto self = captures[0].get("constructor").getPrototype().construct({}, newTarget);
        self.set("shaderNode", arg(args, 0)); self.set("rawInputs", arg(args, 1)); self.set("isShaderCallNodeInternal", true); return self;
    }, {callHolder}, "ShaderCallNodeInternal", 2);
    callHolder.set("constructor", callConstructor); callConstructor.setPrototype(base);
    auto callPrototype = callConstructor.get("prototype"); callPrototype.setPrototype(base.get("prototype"));
    installMethods(callPrototype, shaderCallMethod, helpers, {{"isCacheable", 1}, {"generateNodeType", 1}, {"getElementType", 1},
        {"getMemberType", 2}, {"call", 1}, {"setupOutput", 1}, {"getOutputNode", 1}, {"build", 1}});

    auto shaderHolder = JSValue::object(nullptr);
    auto shaderConstructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) {
        auto self = captures[0].get("constructor").getPrototype().construct({arg(args, 1)}, newTarget);
        self.set("jsFunc", arg(args, 0)); self.set("layout", nullptr); self.set("global", true); self.set("once", false); return self;
    }, {shaderHolder}, "ShaderNodeInternal", 2);
    shaderHolder.set("constructor", shaderConstructor); shaderConstructor.setPrototype(base);
    auto shaderPrototype = shaderConstructor.get("prototype"); shaderPrototype.setPrototype(base.get("prototype"));
    installMethods(shaderPrototype, shaderMethod, callConstructor, {{"isCacheable", 0}, {"setLayout", 1}, {"getLayout", 0}, {"call", 0}, {"setup", 0}});
    auto shaderExport = JSValue::constructor([](const JSValue&, const Args& args, const Args& captures) {
        return captures[0].construct({arg(args, 0), arg(args, 1)});
    }, {shaderConstructor}, "ShaderNode", 2, JSValue::ConstructorKind::Callable);
    own(module, "ShaderNode", shaderExport);

    auto state = literal({{"fnId", 0}}); auto fnHolder = JSValue::object(nullptr);
    auto fnConstructor = JSValue::builtinConstructor([](const JSValue& newTarget, const Args& args, const Args& captures) {
        auto self = captures[0].get("constructor").getPrototype().construct({}, newTarget);
        const auto jsFunc = arg(args, 0); auto layout = arg(args, 1, nullptr); JSValue nodeType = nullptr;
        if (!layout.isNull()) {
            if (layout.isObject() && !layout.isCallable()) nodeType = layout.get("return");
            else {
                if (layout.type() == JSValue::Type::String) nodeType = layout;
                else ThreeUtils::error({"TSL: Invalid layout type.", nativeStackTraceConstructor().construct()});
                layout = nullptr;
            }
        }
        self.set("shaderNode", captures[1].construct({jsFunc, nodeType}));
        if (!layout.isNull()) invoke(self, "setLayout")({layout});
        self.set("isFn", true); return self;
    }, {fnHolder, shaderExport}, "FnNode", 1);
    fnHolder.set("constructor", fnConstructor); fnConstructor.setPrototype(base);
    auto fnPrototype = fnConstructor.get("prototype"); fnPrototype.setPrototype(base.get("prototype"));
    installMethods(fnPrototype, fnMethod, state, {{"isCacheable", 0}, {"setLayout", 1}, {"generateNodeType", 1}, {"call", 0}, {"once", 0}, {"generate", 1}});
    own(module, "Fn", JSValue::constructor([](const JSValue&, const Args& args, const Args& captures) {
        const auto instance = captures[0].construct({arg(args, 0), arg(args, 1, nullptr)});
        auto target = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
        auto handler = literal();
        own(handler, "apply", JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto method = invoke(captures[0], "call"); Args params;
            each(arg(args, 2), [&](const JSValue& value) { params.push_back(value); });
            return method(params);
        }, {instance}, "apply", 3));
        own(handler, "get", JSValue::function([](const JSValue&, const Args& args, const Args& captures) { return captures[0].get(arg(args, 1), arg(args, 2)); }, {instance}, "get", 3));
        own(handler, "set", JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue { return captures[0].reflectSet(arg(args, 1), arg(args, 2), arg(args, 3)); }, {instance}, "set", 4));
        return createNativeProxy(target, handler);
    }, {fnConstructor}, "Fn", 1, JSValue::ConstructorKind::Callable));
    own(module, "setCurrentStack", JSValue::function([](const JSValue&, const Args& args, const Args&) -> JSValue { nativeSetCurrentStack(arg(args, 0)); return {}; }, {}, "setCurrentStack", 1));
    own(module, "getCurrentStack", JSValue::function([](const JSValue&, const Args&, const Args&) { return nativeGetCurrentStack(); }, {}, "getCurrentStack", 0));
    for (const auto* name : {"If", "Switch"}) own(module, name, JSValue::function([](const JSValue&, const Args& args, const Args& captures) {
        return invoke(nativeGetCurrentStack(), captures[0].string().c_str())(args);
    }, {name}, name, 0));
    const auto stack = JSValue::constructor([](const JSValue&, const Args& args, const Args&) {
        const auto node = arg(args, 0), current = nativeGetCurrentStack();
        if (current.toBoolean()) invoke(current, "addToStack")({node}); return node;
    }, {}, "Stack", 1, JSValue::ConstructorKind::Callable);
    own(module, "Stack", stack);
    module.get("addMethodChaining").call({}, {"toStack", stack});
}
