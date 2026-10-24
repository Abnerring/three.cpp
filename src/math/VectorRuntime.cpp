#include "math/Vector2.h"
#include "math/Vector3.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue add(JSValue a, JSValue b) { const auto x = a.toPrimitive(), y = b.toPrimitive(); if (x.type() == JSValue::Type::String || y.type() == JSValue::Type::String) return x.toString() + y.toString(); return x.toNumber() + y.toNumber(); }
double product(const JSValue& a, const JSValue& b) { const auto left = a.toNumber(); const auto right = b.toNumber(); return left * right; }
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto count = static_cast<unsigned>(captures[1].number()); const auto source = arg(args, 0);
    if (name == "get width" || name == "get height") return self.get(name == "get width" ? "x" : "y");
    if (name == "set width" || name == "set height") { self.set(name == "set width" ? "x" : "y", source); return {}; }
    if (name == "clone") { const auto ctor = self.get("constructor"); Args values; for (unsigned i = 0; i < count; ++i) values.push_back(self.get(std::string(1, "xyz"[i]))); return ctor.construct(values); }
    if (name == "set" || name == "setScalar" || name == "copy") {
        auto z = arg(args, 2); if (name == "set" && count == 3 && z.isUndefined()) z = self.get("z");
        for (unsigned i = 0; i < count; ++i) { const std::string key(1, "xyz"[i]); self.set(key, name == "copy" ? source.get(key) : name == "setScalar" ? source : i == 2 ? z : arg(args, i)); } return self;
    }
    if (name == "toArray" || name == "fromArray") {
        auto array = source; if (name == "toArray" && array.isUndefined()) array = JSValue::array(); const auto raw = arg(args, 1), offset = raw.isUndefined() ? JSValue(0) : raw;
        for (unsigned i = 0; i < count; ++i) { const auto index = i ? add(offset, i) : offset; const std::string key(1, "xyz"[i]); if (name == "toArray") array.set(index, self.get(key)); else self.set(key, array.get(index)); }
        return name == "toArray" ? array : self;
    }
    if (name == "equals") { for (unsigned i = 0; i < count; ++i) { const std::string key(1, "xyz"[i]); const auto left = source.get(key); if (!left.strictEquals(self.get(key))) return false; } return true; }
    if (name == "applyMatrix3") {
        const auto x = self.get("x"), y = self.get("y"); const auto z = count == 3 ? self.get("z") : JSValue{}; const auto elements = source.get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto a = product(elements.get(JSValue(i)), x);
            const auto b = product(elements.get(JSValue(i + 3)), y);
            const auto c = elements.get(JSValue(i + 6));
            self.set(std::string(1, "xyz"[i]), count == 3 ? JSValue((a + b) + product(c, z)) : add(a + b, c));
        }
        return self;
    }
    throw std::logic_error("Unknown managed vector method");
}
JSValue constructor(unsigned count) {
    static thread_local JSWeakValue weak[2]; static thread_local bool initializing[2]{};
    const auto index = count - 2; if (const auto ctor = weak[index].lock(); ctor && ctor->isConstructible()) return *ctor;
    const auto name = count == 2 ? "Vector2" : "Vector3"; JSModuleInitializationScope scope(initializing[index], name);
    auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue { auto self = receiver; const auto count = static_cast<unsigned>(captures[0].number()); for (unsigned i = 0; i < count; ++i) { const auto value = arg(args, i); self.set(std::string(1, "xyz"[i]), value.isUndefined() ? JSValue(0) : value); } return {}; }, {count}, name, 0);
    auto prototype = ctor.get("prototype");
    if (count == 2) for (const auto* key : {"width", "height"}) { JSPropertyDescriptor d; d.configurable = true; d.getter = JSValue::function(method, {std::string("get ") + key, count}, std::string("get ") + key, 0); d.setter = JSValue::function(method, {std::string("set ") + key, count}, std::string("set ") + key, 1); prototype.defineProperty(key, d); }
    for (const auto& [methodName, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"set", count}, {"setScalar", 1}, {"clone", 0}, {"copy", 1}, {"applyMatrix3", 1}, {"equals", 1}, {"fromArray", 1}, {"toArray", 0}}) { JSPropertyDescriptor d; d.value = JSValue::function(method, {methodName, count}, methodName, arity); d.writable = d.configurable = true; prototype.defineProperty(methodName, d); }
    prototype.set(std::string("is") + name, true); ctor.retainForHeapLifetime(); weak[index] = JSWeakValue(ctor); return ctor;
}
}
JSValue nativeVector2Constructor() { return constructor(2); }
JSValue nativeVector3Constructor() { return constructor(3); }
