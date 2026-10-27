#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLCoreRuntime.h"
#include "nodes/core/ConstNode.h"
#include "nodes/core/NodeUtils.h"
#include "core/JSMap.h"
#include "core/JSIterator.h"
#include "core/JSModuleObject.h"
#include <numbers>
#include <algorithm>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i, const JSValue& fallback = {}) { return i < args.size() && !args[i].isUndefined() ? args[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSValue constNode(const JSValue& value, const JSValue& type, const JSValue& cache, const JSValue& constructor) {
    if (invoke(cache, "has")({value}).toBoolean()) return invoke(cache, "get")({value});
    if (value.get("isNode").strictEquals(true)) return value;
    auto node = constructor.construct({value, type}); node.set("isWeak", !type.toBoolean() && value.type() == JSValue::Type::Number); return node;
}
}
JSValue nativeTSLCore() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "TSLCore");
    auto module = JSValue::object(); const auto constructor = nativeConstNodeConstructor();
    auto boolsCache = createNativeMap();
    for (const bool value : {false, true}) invoke(boolsCache, "set")({value, constructor.construct({value})});
    auto uintsCache = createNativeMap();
    for (const double value : {0.0, 1.0, 2.0, 3.0}) invoke(uintsCache, "set")({value, constructor.construct({value, "uint"})});
    const auto forKeys = [](const JSValue& map, const auto& callback) {
        auto iterator = getJSIterator(invoke(map, "keys")());
        while (const auto value = stepJSIterator(iterator)) {
            try { callback(*value); } catch (...) { closeJSIterator(iterator, std::current_exception()); }
        }
    };
    auto intsCache = createNativeMap();
    forKeys(uintsCache, [&](const JSValue& value) { invoke(intsCache, "set")({value, constructor.construct({value, "int"})}); });
    for (const double value : {-1.0, -2.0}) invoke(intsCache, "set")({value, constructor.construct({value, "int"})});
    auto floatsCache = createNativeMap();
    forKeys(intsCache, [&](const JSValue& value) { invoke(floatsCache, "set")({value, constructor.construct({value})}); });
    constexpr double pi = std::numbers::pi_v<double>;
    const auto floats = {0.5, 1.5, 1.0 / 3.0, 1e-6, 1e6, pi, pi * 2.0, 1.0 / pi, 2.0 / pi, 1.0 / (pi * 2.0), pi / 2.0};
    for (const double value : floats) invoke(floatsCache, "set")({value, constructor.construct({value})});
    for (const double value : floats) invoke(floatsCache, "set")({-value, constructor.construct({-value})});
    auto cacheMaps = JSValue::object();
    for (const auto& [key, value] : std::initializer_list<std::pair<const char*, JSValue>>{{"bool", boolsCache}, {"uint", uintsCache}, {"ints", intsCache}, {"float", floatsCache}}) {
        JSPropertyDescriptor descriptor; descriptor.value = value; descriptor.writable = descriptor.enumerable = descriptor.configurable = true; cacheMaps.defineProperty(key, descriptor);
    }
    const auto cache = nativeMapConstructor().construct({boolsCache});
    forKeys(floatsCache, [&](const JSValue& value) { invoke(cache, "set")({value, constNode(value, {}, cache, constructor)}); });
    const auto install = [&](const char* name, const JSValue& function) { JSPropertyDescriptor d; d.value = function; d.writable = d.enumerable = d.configurable = true; module.defineProperty(name, d); };
    install("getConstNodeType", JSValue::function([](const JSValue&, const Args& args, const Args&) -> JSValue {
        const auto value = arg(args, 0); if (value.isUndefined() || value.isNull()) return nullptr;
        const auto nodeType = value.get("nodeType"); if (nodeType.toBoolean()) return nodeType;
        const auto convertTo = value.get("convertTo"); if (convertTo.toBoolean()) return convertTo;
        return value.type() == JSValue::Type::String ? value : JSValue(nullptr);
    }, {}, "getConstNodeType", 1));
    install("nodeObject", JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue {
        const auto value = arg(args, 0), altType = arg(args, 1, nullptr), type = captures[2].call({}, {value});
        if (type.strictEquals("node")) return value;
        if ((altType.isNull() && (type.strictEquals("float") || type.strictEquals("boolean"))) ||
            (type.toBoolean() && !type.strictEquals("shader") && !type.strictEquals("string")))
            return constNode(value, altType, captures[0], captures[1]);
        if (type.strictEquals("shader")) {
            if (value.get("isFn").toBoolean()) return value;
            return captures[4].get("Fn").call({}, {value});
        }
        return value;
    }, {cache, constructor, nativeNodeUtils().get("getValueType"), cacheMaps, module}, "nodeObject", 1));
    const auto getConstNode = JSValue::function([](const JSValue&,const Args& args,const Args& captures)->JSValue {
        return constNode(arg(args,0),arg(args,1),captures[0],captures[1]);
    },{cache,constructor},"getConstNode",2);
    installNativeTSLCoreExtensions(module,getConstNode,cacheMaps);
    // Export a separate namespace object. Internal closures hold the lexical
    // bindings, so replacing an exported property cannot replace an import.
    auto exports=JSValue::object();auto names=module.ownPropertyNames();std::sort(names.begin(),names.end());
    for(const auto& name:names){JSPropertyDescriptor d;d.value=module.get(name);d.writable=d.enumerable=d.configurable=true;exports.defineProperty(name,d);}
    exports.retainForHeapLifetime(); weak = JSWeakValue(exports); return exports;
}
