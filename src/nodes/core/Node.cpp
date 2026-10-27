#include "nodes/core/Node.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/constants.h"
#include "nodes/core/StackTrace.h"
#include "core/EventDispatcher.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"
#include "core/JSModuleObject.h"
#include "core/JSSet.h"
#include "math/MathUtils.h"
#include "utils.h"
#include <cmath>
#include <unordered_set>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
struct Invocation {
    JSValue object, method;
    JSValue operator()(const Args& args = {}) const { return method.call(object, args); }
};
Invocation invoke(const JSValue& object, const char* name) { return {object, object.get(name)}; }
JSValue defaulted(const Args& args, std::size_t i, const JSValue& fallback) {
    const auto value = arg(args, i); return value.isUndefined() ? fallback : value;
}
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor; descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue literal(std::initializer_list<std::pair<const char*, JSValue>> fields) {
    auto object = JSValue::object(); for (const auto& [name, value] : fields) own(object, name, value); return object;
}
void push(JSValue array, const JSValue& value) { invoke(array, "push")({value}); }
template<class F> void each(const JSValue& iterable, F body) {
    auto iterator = getJSIterator(iterable);
    while (const auto next = stepJSIterator(iterator)) {
        try { body(*next); } catch (...) { closeJSIterator(iterator, std::current_exception()); }
    }
}
// Object.values snapshots keys, then evaluates all enumerable values before the
// caller begins iterating. Descriptors are rechecked after preceding getters.
JSValue values(const JSValue& object) {
    if (object.isNull() || object.isUndefined()) throw JSTypeError("Cannot convert null or undefined to object");
    auto out = JSValue::array();
    if (!object.isObject()) {
        if (object.type() == JSValue::Type::String)
            for (std::uint32_t i = 0; i < object.get("length").toNumber(); ++i) own(out, std::to_string(i).c_str(), object.get(std::to_string(i)));
        return out;
    }
    for (const auto& key : object.ownPropertyNames()) {
        const auto descriptor = object.getOwnPropertyDescriptor(key);
        if (descriptor && descriptor->enumerable.value_or(false)) {
            const auto value = object.get(key);
            own(out, std::to_string(out.length()).c_str(), value);
        }
    }
    return out;
}
template<class F> void forIn(JSValue object, F body) {
    if (object.isNull() || object.isUndefined()) return;
    if (!object.isObject()) {
        if (object.type() == JSValue::Type::String)
            for (std::uint32_t i = 0; i < object.get("length").toNumber(); ++i) body(JSValue(std::to_string(i)));
        return;
    }
    std::unordered_set<std::string> visited;
    std::vector<std::string> candidates;
    auto level = object;
    while (!level.isNull()) {
        for (const auto& key : level.ownPropertyNames()) {
            if (visited.contains(key)) continue;
            const auto descriptor = level.getOwnPropertyDescriptor(key);
            if (!descriptor) continue;
            visited.insert(key);
            if (descriptor->enumerable.value_or(false)) candidates.push_back(key);
        }
        level = level.getPrototype();
    }
    // Match the pinned V8 for-in snapshot: later added prototype keys wait for
    // the next enumeration, while deletion still suppresses an initial key.
    for (const auto& key : candidates) if (object.has(key)) body(JSValue(key));
}
JSValue hashCall(const char* name, const Args& args) {
    return nativeNodeUtilsBinding(name).call({}, args);
}
JSValue children(JSValue self, JSValue ignores) {
    auto result = JSValue::array();
    invoke(ignores, "add")({self});
    for (const auto& property : self.ownPropertyNames()) {
        const auto object = self.get(property);
        // This read precedes the private-name check in the source.
        if (property.starts_with('_') || invoke(ignores, "has")({object}).toBoolean()) continue;
        if (object.type() == JSValue::Type::Array) {
            for (double i = 0; i < object.get("length").toNumber(); ++i) {
                const auto child = object.get(JSValue(i));
                if (child.toBoolean() && child.get("isNode").strictEquals(true))
                    push(result, literal({{"property", property}, {"index", i}, {"childNode", child}}));
            }
        } else if (object.toBoolean() && object.get("isNode").strictEquals(true)) {
            push(result, literal({{"property", property}, {"childNode", object}}));
        } else if (object.isObject() && object.getPrototype().strictEquals(JSValue::intrinsicObjectConstructor().get("prototype"))) {
            forIn(object, [&](const JSValue& key) {
                if (key.string().starts_with('_')) return;
                const auto child = object.get(key);
                if (child.toBoolean() && child.get("isNode").strictEquals(true))
                    push(result, literal({{"property", property}, {"index", key}, {"childNode", child}}));
            });
        }
    }
    return result;
}
constexpr const char* childGeneratorBrand = "Node.getChildren Generator";
void finishGenerator(JSValue state) {
    state.set("executing", false); state.set("done", true);
    // The pinned V8 generator retains its receiver while the iterator is live,
    // including after return/throw. Keep this managed edge until collection.
    state.set("iterator", {}); state.set("next", {});
}
JSValue resumeGenerator(const JSValue& receiver, const Args& args, const Args& captures) {
    if (!receiver.isHostObject(childGeneratorBrand)) throw JSTypeError("Incompatible Generator receiver");
    auto state = receiver.getHostSlot("state");
    if (state.get("executing").toBoolean()) throw JSTypeError("Generator is already running");
    const auto& operation = captures[0].string();
    if (operation != "next") {
        if (state.get("done").toBoolean()) {
            if (operation == "throw") throw JSThrownValue(arg(args, 0));
            return literal({{"value", arg(args, 0)}, {"done", true}});
        }
        state.set("executing", true);
        try {
            if (state.get("started").toBoolean()) {
                JSIteratorRecord inner{state.get("iterator"), state.get("next")};
                if (operation == "throw") closeJSIterator(inner, std::make_exception_ptr(JSThrownValue(arg(args, 0))));
                closeJSIterator(inner);
            }
            finishGenerator(state);
            if (operation == "throw") throw JSThrownValue(arg(args, 0));
            return literal({{"value", arg(args, 0)}, {"done", true}});
        } catch (...) { finishGenerator(state); throw; }
    }
    if (state.get("done").toBoolean()) return literal({{"value", {}}, {"done", true}});
    state.set("executing", true);
    try {
        if (!state.get("started").toBoolean()) {
            state.set("started", true);
            auto inner = getJSIterator(invoke(state.get("self"), "_getChildren")());
            state.set("iterator", inner.iterator); state.set("next", inner.nextMethod);
        }
        JSIteratorRecord inner{state.get("iterator"), state.get("next")};
        const auto value = stepJSIterator(inner);
        if (!value) { finishGenerator(state); return literal({{"value", {}}, {"done", true}}); }
        try {
            const auto child = value->get("childNode"); state.set("executing", false);
            return literal({{"value", child}, {"done", false}});
        } catch (...) { closeJSIterator(inner, std::current_exception()); }
    } catch (...) { finishGenerator(state); throw; }
}
JSValue childGeneratorPrototype() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    auto generatorConstructor = JSValue::callableBuiltinConstructor(
        [](const JSValue&, const JSValue&, const Args&, const Args&) -> JSValue {
            throw std::logic_error("Dynamic JavaScript generator source is not supported by the native C++ runtime");
        }, {}, "GeneratorFunction", 1);
    auto functionPrototype = generatorConstructor.get("prototype");
    const auto functionBase = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return {}; }).getPrototype();
    functionPrototype.setPrototype(functionBase);
    generatorConstructor.setPrototype(functionBase.get("constructor"));
    auto prototype = JSValue::object(nativeIteratorPrototype());
    const auto define = [](JSValue object, const JSValue& key, const JSValue& value, bool writable) {
        JSPropertyDescriptor descriptor; descriptor.value = value; descriptor.writable = writable; descriptor.configurable = true;
        object.defineProperty(key, descriptor);
    };
    define(functionPrototype, "prototype", prototype, false);
    define(functionPrototype, "constructor", generatorConstructor, false);
    define(functionPrototype, JSValue::wellKnownSymbol("toStringTag"), "GeneratorFunction", false);
    define(prototype, "constructor", functionPrototype, false);
    for (const auto* name : {"next", "return", "throw"})
        define(prototype, name, JSValue::function(resumeGenerator, {name}, name, 1), true);
    define(prototype, JSValue::wellKnownSymbol("toStringTag"), "Generator", false);
    prototype.retainForHeapLifetime(); weak = JSWeakValue(prototype); return prototype;
}
JSValue childIterator(const JSValue& self, const JSValue& function) {
    // The generator body begins at the first next(), and state is an internal
    // managed edge rather than an enumerable field or per-instance method.
    auto state = literal({{"self", self}, {"started", false}, {"done", false}, {"executing", false}});
    auto token = std::make_shared<unsigned>(0);
    auto iterator = JSValue::hostObject(token, token.get(), childGeneratorBrand);
    const auto prototype = function.get("prototype");
    iterator.setPrototype(prototype.isObject() ? prototype : childGeneratorPrototype());
    iterator.setHostSlot("state", state); return iterator;
}
JSValue build(JSValue self, JSValue builder, JSValue output) {
    const auto ref = invoke(self, "getShared")({builder});
    if (!self.strictEquals(ref)) return invoke(ref, "build")({builder, output});
    if (!self.get("_beforeNodes").isNull()) {
        const auto before = self.get("_beforeNodes"); self.set("_beforeNodes", nullptr);
        each(before, [&](const JSValue& node) { invoke(node, "build")({builder, output}); });
        self.set("_beforeNodes", before);
    }
    auto data = invoke(builder, "getDataFromNode")({self});
    { const auto prior = data.get("buildStages"); data.set("buildStages", prior.toBoolean() ? prior : JSValue::object()); }
    { auto stages = data.get("buildStages"); stages.set(builder.get("buildStage"), true); }
    // A normal module object is used in the source, so inherited keys remain
    // observable for nonstandard build-stage values.
    static thread_local JSWeakValue weakStages;
    auto stages = weakStages.lock();
    if (!stages || !stages->isObject()) { auto value = literal({{"analyze", "setup"}, {"generate", "analyze"}}); value.retainForHeapLifetime(); weakStages = JSWeakValue(value); stages = value; }
    const auto parent = stages->get(builder.get("buildStage"));
    if (parent.toBoolean() && !data.get("buildStages").get(parent).strictEquals(true)) {
        const auto previous = invoke(builder, "getBuildStage")();
        invoke(builder, "setBuildStage")({parent}); invoke(self, "build")({builder}); invoke(builder, "setBuildStage")({previous});
    }
    invoke(builder, "addChain")({self});
    JSValue result = nullptr;
    const auto stage = invoke(builder, "getBuildStage")();
    if (stage.strictEquals("setup")) {
        invoke(builder, "addNode")({self}); invoke(self, "updateReference")({builder});
        auto properties = invoke(builder, "getNodeProperties")({self});
        if (!properties.get("initialized").strictEquals(true)) {
            properties.set("initialized", true);
            const auto setup = invoke(self, "setup")({builder});
            const auto value = setup.toBoolean() ? setup : properties.get("outputNode");
            properties.set("outputNode", value.toBoolean() ? value : JSValue(nullptr));
            each(values(properties), [&](const JSValue& child) {
                if (child.toBoolean() && child.get("isNode").strictEquals(true)) {
                    if (child.get("parents").strictEquals(true)) {
                        auto childProperties = invoke(builder, "getNodeProperties")({child});
                        const auto parents = childProperties.get("parents");
                        childProperties.set("parents", parents.toBoolean() ? parents : JSValue::array());
                        invoke(childProperties.get("parents"), "push")({self});
                    }
                    invoke(child, "build")({builder});
                }
            });
            invoke(builder, "addSequentialNode")({self});
        }
        result = properties.get("outputNode");
    } else if (stage.strictEquals("analyze")) {
        invoke(self, "analyze")({builder, output});
    } else if (stage.strictEquals("generate")) {
        if (!data.get("flowBlock").isUndefined()) {
            auto block = builder.get("flowBlock");
            while (!block.isNull() && !block.strictEquals(data.get("flowBlock"))) block = block.get("parent");
            if (block.isNull()) { data.set("flowBlock", {}); data.set("propertyName", {}); data.set("snippet", {}); data.set("generated", {}); }
        }
        const bool cached = !data.get("propertyName").isUndefined() || !data.get("snippet").isUndefined();
        const auto codeLength = builder.get("flow").get("code").get("length");
        const auto cacheable = invoke(self, "isCacheable")({builder});
        const JSValue allowed = cacheable.toBoolean() ? JSValue(invoke(builder, "isReference")({output}).strictEquals(false)) : cacheable;
        JSValue type = nullptr;
        if (allowed.toBoolean()) type = invoke(builder, "getVectorType")({invoke(self, "getNodeType")({builder, output})});
        bool cacheResult = false;
        if (allowed.toBoolean() && !type.strictEquals("void") && !output.strictEquals("void")) cacheResult = data.get("usageCount").toNumber() > 1;
        const auto generateOutput = cacheResult ? type : output;
        if (allowed.toBoolean() && !data.get("propertyName").isUndefined()) {
            result = invoke(builder, "format")({data.get("propertyName"), type, output});
        } else {
            if (self.get("generate").get("length").toNumber() < 2) {
                const auto onceType = invoke(self, "getNodeType")({builder});
                auto onceData = invoke(builder, "getDataFromNode")({self}); result = onceData.get("snippet");
                if (result.isUndefined()) {
                    if (onceData.get("generated").isUndefined()) {
                        onceData.set("generated", true);
                        const auto generated = invoke(self, "generate")({builder}); result = generated.toBoolean() ? generated : JSValue("");
                        onceData.set("snippet", result);
                    } else { ThreeUtils::warn({"Node: Recursion detected.", self}); result = "/* Recursion detected. */"; }
                }
                result = invoke(builder, "format")({result, onceType, generateOutput});
            } else { const auto generated = invoke(self, "generate")({builder, generateOutput}); result = generated.toBoolean() ? generated : JSValue(""); }
            if (result.strictEquals("") && !generateOutput.isNull() && !generateOutput.strictEquals("void") && !generateOutput.strictEquals("OutputType")) {
                ThreeUtils::error({"TSL: Invalid generated code, expected a \"" + generateOutput.toString() + "\"."});
                result = invoke(builder, "generateConst")({generateOutput});
            }
            if (cacheResult) {
                const bool readOnly = !data.get("assign").strictEquals(true);
                const auto variable = invoke(builder, "getVarFromNode")({self, nullptr, type, {}, readOnly, true, "cacheVariable"});
                const auto propertyName = invoke(builder, "getPropertyName")({variable});
                const auto count = invoke(self, "getArrayCount")({builder});
                const auto declaration = invoke(builder, readOnly ? "generateLetStatement" : "generateVarStatement")({variable.get("type"), propertyName, count});
                invoke(builder, "addLineFlowCode")({declaration.toString() + " = " + result.toString(), self});
                data.set("snippet", result); data.set("propertyName", propertyName);
                result = invoke(builder, "format")({propertyName, type, output});
            }
        }
        const bool local = invoke(self, "isGlobal")({builder}).strictEquals(false) || !builder.get("flow").get("code").get("length").strictEquals(codeLength);
        if (!cached && (!data.get("propertyName").isUndefined() || !data.get("snippet").isUndefined()) && local && !builder.get("flowBlock").isNull())
            data.set("flowBlock", builder.get("flowBlock"));
    }
    invoke(builder, "removeChain")({self});
    return result;
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto& name = captures[0].string();
    if (name == "isCacheable") return true;
    if (name == "needsUpdate") { if (arg(args, 0).strictEquals(true)) self.set("version", self.get("version").toNumber() + 1); return {}; }
    if (name == "uuid") { if (self.get("_uuid").isNull()) self.set("_uuid", MathUtils::generateUUID()); return self.get("_uuid"); }
    if (name == "type") return self.get("constructor").get("type");
    if (name == "onUpdate") {
        self.set("updateType", arg(args, 1));
        const auto callback = arg(args, 0); const auto bound = invoke(callback, "bind")({self}); self.set("update", bound); return self;
    }
    if (name == "onFrameUpdate" || name == "onRenderUpdate" || name == "onObjectUpdate") {
        const auto update = invoke(self, "onUpdate");
        const auto type = nativeNodeConstantsBinding("NodeUpdateType").get(
            name == "onFrameUpdate" ? "FRAME" : name == "onRenderUpdate" ? "RENDER" : "OBJECT");
        return update({arg(args, 0), type});
    }
    if (name == "onReference") { const auto bound = invoke(arg(args, 0), "bind")({self}); self.set("updateReference", bound); return self; }
    if (name == "updateReference" || name == "getScope") return self;
    if (name == "isGlobal") return self.get("global");
    if (name == "getChildren") return childIterator(self, captures[1].get("function"));
    if (name == "_getChildren") { const auto ignores = arg(args, 0); return children(self, ignores.isUndefined() ? createNativeSet() : ignores); }
    if (name == "dispose") { invoke(self, "dispatchEvent")({literal({{"type", "dispose"}})}); return {}; }
    if (name == "traverse") {
        const auto callback = arg(args, 0); callback.call({}, {self});
        each(invoke(self, "getChildren")(), [&](const JSValue& child) { invoke(child, "traverse")({callback}); }); return {};
    }
    if (name == "getCacheKey") {
        auto force = defaulted(args, 0, false); auto ignores = defaulted(args, 1, nullptr);
        if (!force.toBoolean()) { const auto version = self.get("version"); const auto prior = self.get("_cacheKeyVersion"); force = !version.strictEquals(prior); }
        if (force.strictEquals(true) || self.get("_cacheKey").isNull()) {
            if (ignores.isNull()) ignores = createNativeSet();
            auto array = JSValue::array();
            each(invoke(self, "_getChildren")({ignores}), [&](const JSValue& record) {
                const auto property = record.get("property"), child = record.get("childNode");
                // Capture push before evaluating its argument expressions.
                const auto pushMethod = array.get("push");
                const auto shortened = invoke(property, "slice")({0, -4});
                const auto propertyHash = hashCall("hashString", {shortened});
                const auto childHash = invoke(child, "getCacheKey")({force, ignores});
                pushMethod.call(array, {propertyHash, childHash});
            });
            const auto childHash = hashCall("hashArray", {array});
            const auto custom = invoke(self, "customCacheKey")(); self.set("_cacheKey", hashCall("hash", {childHash, custom}));
            self.set("_cacheKeyVersion", self.get("version"));
        }
        return self.get("_cacheKey");
    }
    if (name == "customCacheKey") return self.get("id");
    if (name == "getHash") { const auto id = self.get("id"); if (id.isSymbol()) return "Symbol(" + id.symbolDescription().value_or("") + ")"; return id.toString(); }
    if (name == "getUpdateType") return self.get("updateType");
    if (name == "getUpdateBeforeType") return self.get("updateBeforeType");
    if (name == "getUpdateAfterType") return self.get("updateAfterType");
    if (name == "getMemberType") return "void";
    if (name == "getArrayCount") return nullptr;
    if (name == "getElementType") { const auto builder = arg(args, 0); const auto type = invoke(self, "getNodeType")({builder}); return invoke(builder, "getElementType")({type}); }
    if (name == "getNodeType") {
        const auto builder = arg(args, 0), output = defaulted(args, 1, nullptr);
        auto data = invoke(builder, "getDataFromNode")({self}); JSValue type;
        if (!output.isNull()) {
            const auto prior = data.get("typeFromOutput"); data.set("typeFromOutput", prior.toBoolean() ? prior : JSValue::object());
            type = data.get("typeFromOutput").get(output);
            if (type.isUndefined()) {
                type = invoke(self, "generateNodeType")({builder, output});
                auto target = data.get("typeFromOutput"); target.set(output, type);
            }
        } else {
            type = data.get("type"); if (type.isUndefined()) { type = invoke(self, "generateNodeType")({builder}); data.set("type", type); }
        }
        return type;
    }
    if (name == "generateNodeType") {
        const auto builder = arg(args, 0), output = defaulted(args, 1, nullptr);
        const auto properties = invoke(builder, "getNodeProperties")({self});
        if (properties.get("outputNode").toBoolean()) return invoke(properties.get("outputNode"), "getNodeType")({builder, output});
        return self.get("nodeType");
    }
    if (name == "getShared") {
        const auto builder = arg(args, 0); const auto hash = invoke(self, "getHash")({builder});
        const auto found = invoke(builder, "getNodeFromHash")({hash}); JSValue shared = nullptr;
        if (found.toBoolean() && !found.strictEquals(self)) shared = found;
        else if (builder.get("context").get("overrideNodes").toBoolean()) {
            const auto callback = invoke(builder.get("context").get("overrideNodes"), "get")({self});
            if (callback.toBoolean()) {
                auto data = invoke(builder, "getDataFromNode")({self});
                if (!data.get("isOverwritten").strictEquals(true)) {
                    data.set("isOverwritten", true);
                    shared = invoke(callback.call({}, {builder}), "overrideNode")({self, nullptr}); data.set("sharedNode", shared);
                } else shared = data.get("sharedNode");
            }
        }
        return shared.toBoolean() ? shared : self;
    }
    if (name == "setup") {
        const auto builder = arg(args, 0); auto properties = invoke(builder, "getNodeProperties")({self}); double index = 0;
        each(invoke(self, "getChildren")(), [&](const JSValue& child) { const auto key = "node" + JSValue(index++).toString(); properties.set(key, child); });
        const auto output = properties.get("outputNode"); return output.toBoolean() ? output : JSValue(nullptr);
    }
    if (name == "analyze") {
        const auto builder = arg(args, 0), output = defaulted(args, 1, nullptr); const auto usage = invoke(builder, "increaseUsage")({self});
        if (self.get("parents").strictEquals(true)) {
            auto data = invoke(builder, "getDataFromNode")({self, "any"});
            const auto stages = data.get("stages"); data.set("stages", stages.toBoolean() ? stages : JSValue::object());
            auto target = data.get("stages"); const auto key = builder.get("shaderStage");
            const auto prior = data.get("stages").get(builder.get("shaderStage")); target.set(key, prior.toBoolean() ? prior : JSValue::array());
            invoke(data.get("stages").get(builder.get("shaderStage")), "push")({output});
        }
        if (usage.strictEquals(1)) each(values(invoke(builder, "getNodeProperties")({self})), [&](const JSValue& child) {
            if (child.toBoolean() && child.get("isNode").strictEquals(true)) invoke(child, "build")({builder, self});
        });
        return {};
    }
    if (name == "generate") {
        const auto builder = arg(args, 0); const auto output = invoke(builder, "getNodeProperties")({self}).get("outputNode");
        if (output.toBoolean() && output.get("isNode").strictEquals(true)) return invoke(output, "build")({builder, arg(args, 1)}); return {};
    }
    if (name == "update" || name == "updateBefore" || name == "updateAfter") { ThreeUtils::warn({"Abstract function."}); return {}; }
    if (name == "before") { if (self.get("_beforeNodes").isNull()) self.set("_beforeNodes", JSValue::array()); invoke(self.get("_beforeNodes"), "push")({arg(args, 0)}); return self; }
    if (name == "build") return build(self, arg(args, 0), defaulted(args, 1, nullptr));
    if (name == "getSerializeChildren") return invoke(self, "_getChildren")();
    if (name == "serialize") {
        auto json = arg(args, 0); const auto children = invoke(self, "getSerializeChildren")(); auto input = JSValue::object();
        each(children, [&](const JSValue& record) {
            const auto property = record.get("property"), index = record.get("index"), child = record.get("childNode");
            if (!index.isUndefined()) {
                if (input.get(property).isUndefined()) {
                    const bool integer = index.type() == JSValue::Type::Number && std::isfinite(index.number()) && std::trunc(index.number()) == index.number();
                    input.set(property, integer ? JSValue::array() : JSValue::object());
                }
                auto target = input.get(property); const auto key = index;
                const auto uuid = invoke(child, "toJSON")({json.get("meta")}).get("uuid"); target.set(key, uuid);
            } else { const auto uuid = invoke(child, "toJSON")({json.get("meta")}).get("uuid"); input.set(property, uuid); }
        });
        if (!input.keys().empty()) json.set("inputNodes", input); return {};
    }
    if (name == "deserialize") {
        const auto json = arg(args, 0);
        if (!json.get("inputNodes").isUndefined()) {
            const auto nodes = json.get("meta").get("nodes");
            forIn(json.get("inputNodes"), [&](const JSValue& property) {
                if (json.get("inputNodes").get(property).type() == JSValue::Type::Array) {
                    auto input = JSValue::array();
                    each(json.get("inputNodes").get(property), [&](const JSValue& uuid) { const auto pushMethod = input.get("push"); const auto node = nodes.get(uuid); pushMethod.call(input, {node}); });
                    self.set(property, input);
                } else {
                    const auto candidate = json.get("inputNodes").get(property);
                    if (candidate.isNull() || (candidate.isObject() && !candidate.isCallable())) {
                        auto input = JSValue::object();
                        forIn(json.get("inputNodes").get(property), [&](const JSValue& key) {
                            const auto uuid = json.get("inputNodes").get(property).get(key); input.set(key, nodes.get(uuid));
                        });
                        self.set(property, input);
                    } else { const auto uuid = json.get("inputNodes").get(property); self.set(property, nodes.get(uuid)); }
                }
            });
        }
        return {};
    }
    if (name == "toJSON") {
        const auto uuid = self.get("uuid"), type = self.get("type"); auto meta = arg(args, 0);
        const bool root = meta.isUndefined() || meta.type() == JSValue::Type::String;
        if (root) meta = literal({{"textures", JSValue::object()}, {"images", JSValue::object()}, {"nodes", JSValue::object()}});
        auto data = meta.get("nodes").get(uuid);
        if (data.isUndefined()) {
            data = literal({{"uuid", uuid}, {"type", type}, {"meta", meta}, {"metadata", literal({{"version", 4.7}, {"type", "Node"}, {"generator", "Node.toJSON"}})}});
            if (!root) { auto nodes = meta.get("nodes"); nodes.set(data.get("uuid"), data); }
            invoke(self, "serialize")({data}); if (!data.erase("meta")) throw JSTypeError("Cannot delete meta");
        }
        if (root) {
            const auto extract = [&](const JSValue& cache) {
                auto output = JSValue::array();
                forIn(cache, [&](const JSValue& key) { auto item = cache.get(key); if (!item.erase("metadata")) throw JSTypeError("Cannot delete metadata"); push(output, item); });
                return output;
            };
            const auto textures = extract(meta.get("textures")), images = extract(meta.get("images")), nodes = extract(meta.get("nodes"));
            if (textures.length() > 0) data.set("textures", textures);
            if (images.length() > 0) data.set("images", images);
            if (nodes.length() > 0) data.set("nodes", nodes);
        }
        return data;
    }
    throw std::logic_error("Unknown Node method");
}
}

JSValue nativeNodeConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto found = weak.lock(); found && found->isConstructible()) return *found;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "Node");
    const auto updateTypes = nativeNodeConstantsBinding("NodeUpdateType");
    auto state = JSValue::object(nullptr); own(state, "id", 0);
    auto constructor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
        auto self = receiver, state = captures[0];
        self.set("nodeType", defaulted(args, 0, nullptr));
        self.set("updateType", captures[1].get("NONE"));
        self.set("updateBeforeType", captures[1].get("NONE"));
        self.set("updateAfterType", captures[1].get("NONE"));
        self.set("version", 0); self.set("name", ""); self.set("global", false); self.set("parents", false); self.set("isNode", true);
        self.set("_beforeNodes", nullptr); self.set("_cacheKey", nullptr); self.set("_uuid", nullptr); self.set("_cacheKeyVersion", 0);
        const auto id = state.get("id").toNumber(); state.set("id", id + 1); self.set("id", id); self.set("stackTrace", nullptr);
        if (nativeNodeConstructor().get("captureStackTrace").strictEquals(true)) self.set("stackTrace", nativeStackTraceConstructor().construct());
        return {};
    }, {state, updateTypes}, "Node", 0);
    auto prototype = constructor.get("prototype"); prototype.setPrototype(EventDispatcher::runtimePrototype());
    constructor.setPrototype(EventDispatcher::runtimePrototype().get("constructor"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue { return "Node"; }, {}, "get type", 0); type.configurable = true;
    constructor.defineProperty("type", type); own(constructor, "captureStackTrace", false);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"isCacheable", 0}, {"onUpdate", 2}, {"onFrameUpdate", 1}, {"onRenderUpdate", 1}, {"onObjectUpdate", 1}, {"onReference", 1},
        {"updateReference", 0}, {"isGlobal", 0}, {"getChildren", 0}, {"dispose", 0}, {"traverse", 1}, {"_getChildren", 0},
        {"getCacheKey", 0}, {"customCacheKey", 0}, {"getScope", 0}, {"getHash", 0}, {"getUpdateType", 0}, {"getUpdateBeforeType", 0},
        {"getUpdateAfterType", 0}, {"getElementType", 1}, {"getMemberType", 0}, {"getNodeType", 1}, {"generateNodeType", 1}, {"getShared", 1},
        {"getArrayCount", 0}, {"setup", 1}, {"analyze", 1}, {"generate", 2}, {"updateBefore", 0}, {"updateAfter", 0}, {"update", 0},
        {"before", 1}, {"build", 1}, {"getSerializeChildren", 0}, {"serialize", 1}, {"deserialize", 1}, {"toJSON", 1}}) {
        auto holder = JSValue::object(nullptr);
        const bool generator = std::string(name) == "getChildren";
        auto function = JSValue::function(method, generator ? Args{name, holder} : Args{name}, name, arity);
        if (generator) {
            holder.set("function", function);
            function.setPrototype(childGeneratorPrototype().get("constructor"));
            JSPropertyDescriptor instancePrototype; instancePrototype.value = JSValue::object(childGeneratorPrototype());
            instancePrototype.writable = true; function.defineProperty("prototype", instancePrototype);
        }
        JSPropertyDescriptor descriptor; descriptor.value = function;
        descriptor.writable = descriptor.configurable = true; prototype.defineProperty(name, descriptor);
        if (std::string(name) == "isCacheable") {
    for (const auto* name : {"needsUpdate", "uuid", "type"}) {
        JSPropertyDescriptor descriptor; const auto callback = JSValue::function(method, {name}, (std::string(name) == "needsUpdate" ? "set " : "get ") + std::string(name), std::string(name) == "needsUpdate" ? 1 : 0);
        if (std::string(name) == "needsUpdate") descriptor.setter = callback; else descriptor.getter = callback;
        descriptor.configurable = true; prototype.defineProperty(name, descriptor);
    }
        }
    }
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
Node::Node(const JSValue& nodeType) : object_(constructor().construct({nodeType})) {}
JSValue Node::build(const JSValue& builder, const JSValue& output) const { return invoke(object_, "build")({builder, output}); }
JSValue Node::getNodeType(const JSValue& builder, const JSValue& output) const { return invoke(object_, "getNodeType")({builder, output}); }
JSValue Node::getCacheKey(const JSValue& force, const JSValue& ignores) const { return invoke(object_, "getCacheKey")({force, ignores}); }
JSValue Node::toJSON(const JSValue& meta) const { return invoke(object_, "toJSON")({meta}); }
void Node::dispose() const { invoke(object_, "dispose")(); }
