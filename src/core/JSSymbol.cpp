#include "core/JSSymbol.h"

namespace {
using Args = std::vector<JSValue>;
struct SymbolData {};
JSWeakValue& registryWeak() { static thread_local JSWeakValue weak; return weak; }
JSValue registry() {
    if (const auto existing = registryWeak().lock(); existing && existing->isObject()) return *existing;
    auto result = JSValue::object(nullptr);
    result.retainForHeapLifetime();
    registryWeak() = JSWeakValue(result);
    return result;
}
JSPropertyDescriptor data(const JSValue& value, bool writable = true, bool configurable = true) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = writable;
    descriptor.enumerable = false;
    descriptor.configurable = configurable;
    return descriptor;
}
JSValue arg(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
JSValue symbolString(const JSValue& self, const Args&, const Args&) {
    const auto value = nativeSymbolValue(self);
    return "Symbol(" + value.symbolDescription().value_or("") + ")";
}
JSValue symbolValue(const JSValue& self, const Args&, const Args&) { return nativeSymbolValue(self); }
JSValue symbolDescription(const JSValue& self, const Args&, const Args&) {
    const auto description = nativeSymbolValue(self).symbolDescription();
    return description ? JSValue(*description) : JSValue{};
}
}

bool isNativeSymbolObject(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
    return value.isHostObject("three.Symbol");
}
JSValue nativeSymbolValue(const JSValue& receiver) {
    static_cast<void>(JSWeakValue(receiver));
    if (receiver.isSymbol()) return receiver;
    if (receiver.isHostObject("three.Symbol")) return receiver.getHostSlot("SymbolData");
    throw JSTypeError("Symbol method called on incompatible receiver");
}
JSValue createNativeSymbolObject(const JSValue& primitiveSymbol) {
    static_cast<void>(JSWeakValue(primitiveSymbol));
    if (!primitiveSymbol.isSymbol()) throw JSTypeError("SymbolData requires a primitive Symbol");
    const auto prototype = nativeSymbolPrototype();
    auto payload = std::make_shared<SymbolData>();
    auto result = JSValue::hostObject(payload, payload.get(), "three.Symbol");
    // A traced heap edge, never a native JSValue field that would root the symbol.
    result.setHostSlot("SymbolData", primitiveSymbol);
    result.setPrototype(prototype);
    return result;
}
JSValue nativeSymbolFor(const JSValue& key) {
    // Conversion precedes registry access and can reenter Symbol.for.
    const auto text = key.toString();
    auto entries = registry();
    if (entries.hasOwn(text)) return entries.get(text);
    auto result = JSValue::symbol(text);
    entries.set(text, result);
    return result;
}
JSValue nativeSymbolKeyFor(const JSValue& symbol) {
    static_cast<void>(JSWeakValue(symbol));
    if (!symbol.isSymbol()) throw JSTypeError("Symbol.keyFor requires a primitive Symbol");
    const auto entries = registryWeak().lock();
    if (!entries || !entries->isObject()) return {};
    for (const auto& key : entries->ownPropertyKeys())
        if (entries->get(key).sameValue(symbol)) return key;
    return {};
}
bool nativeCanBeHeldWeakly(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
    return value.isObject() || (value.isSymbol() && nativeSymbolKeyFor(value).isUndefined());
}
JSValue nativeSymbolConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto existing = weak.lock(); existing && existing->isCallable()) return *existing;
    // Symbol.prototype is an ordinary object and has no [[SymbolData]].
    auto prototype = JSValue::object();
    // Fixed Node/V8 accepts Symbol as a newTarget while direct construction
    // throws before description conversion. Keep both observable behaviors.
    auto constructor = JSValue::callableBuiltinConstructor(+[](const JSValue&, const JSValue& target, const Args& args, const Args&) -> JSValue {
        if (!target.isUndefined()) throw JSTypeError("Symbol is not a constructor");
        const auto description = arg(args, 0);
        return description.isUndefined() ? JSValue::symbol() : JSValue::symbol(description.toString());
    }, {}, "Symbol", 0, prototype);
    constructor.defineProperty("prototype", data(prototype, false, false));
    prototype.defineProperty("constructor", data(constructor));
    prototype.defineProperty("toString", data(JSValue::function(symbolString, {}, "toString", 0)));
    prototype.defineProperty("valueOf", data(JSValue::function(symbolValue, {}, "valueOf", 0)));
    JSPropertyDescriptor description;
    description.getter = JSValue::function(symbolDescription, {}, "get description", 0);
    description.enumerable = false;
    description.configurable = true;
    prototype.defineProperty("description", description);
    prototype.defineProperty(JSValue::wellKnownSymbol("toStringTag"), data("Symbol", false));
    prototype.defineProperty(JSValue::wellKnownSymbol("toPrimitive"),
        data(JSValue::function(symbolValue, {}, "[Symbol.toPrimitive]", 1), false));
    constructor.defineProperty("for", data(JSValue::function(+[](const JSValue&, const Args& args, const Args&) -> JSValue {
        return nativeSymbolFor(arg(args, 0));
    }, {}, "for", 1)));
    constructor.defineProperty("keyFor", data(JSValue::function(+[](const JSValue&, const Args& args, const Args&) -> JSValue {
        return nativeSymbolKeyFor(arg(args, 0));
    }, {}, "keyFor", 1)));
    for (const auto* name : {"asyncIterator", "hasInstance", "isConcatSpreadable", "iterator", "match", "matchAll",
             "replace", "search", "species", "split", "toPrimitive", "toStringTag", "unscopables", "dispose", "asyncDispose"})
        constructor.defineProperty(name, data(JSValue::wellKnownSymbol(name), false, false));
    constructor.retainForHeapLifetime();
    weak = JSWeakValue(constructor);
    return constructor;
}
JSValue nativeSymbolPrototype() { return nativeSymbolConstructor().get("prototype"); }
