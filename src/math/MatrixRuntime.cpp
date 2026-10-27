#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, unsigned index) { return index < args.size() ? args[index] : JSValue{}; }
JSValue add(const JSValue& a, const JSValue& b) {
    const auto left = a.toPrimitive(); const auto right = b.toPrimitive();
    if (left.type() == JSValue::Type::String || right.type() == JSValue::Type::String)
        return left.toString() + right.toString();
    return left.toNumber() + right.toNumber();
}
Args identity(unsigned width) {
    Args values; for (unsigned i = 0; i < width * width; ++i) values.emplace_back(i % (width + 1) == 0 ? 1 : 0);
    return values;
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    const auto width = static_cast<unsigned>(captures[1].number()), count = width * width;
    if (name == "identity") {
        const auto fn = self.get("set"); fn.call(self, identity(width)); return self;
    }
    if (name == "clone") {
        // Matrix3 uses this.constructor; Matrix4 closes over its class binding.
        const auto ctor = width == 3 ? self.get("constructor") : nativeMatrix4Constructor();
        const auto matrix = ctor.construct(); const auto fn = matrix.get("fromArray");
        const auto elements = self.get("elements"); return fn.call(matrix, {elements});
    }
    if (name == "set") {
        auto elements = self.get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto index = width == 3 ? i : (i % width) * width + i / width;
            const auto parameter = width == 3 ? (i % width) * width + i / width : i;
            elements.set(std::to_string(index), arg(args, parameter));
        }
        return self;
    }
    if (name == "copy" || name == "equals") {
        auto elements = self.get("elements"); const auto source = arg(args, 0).get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto key = std::to_string(i);
            if (name == "copy") elements.set(key, source.get(key));
            else { const auto left = elements.get(key); if (!left.strictEquals(source.get(key))) return false; }
        }
        return name == "copy" ? self : JSValue(true);
    }
    const auto rawOffset = arg(args, 1), offset = rawOffset.isUndefined() ? JSValue(0) : rawOffset;
    auto array = arg(args, 0);
    if (name == "fromArray") {
        for (unsigned i = 0; i < count; ++i) {
            // The property reference is evaluated before the index and RHS on every iteration.
            auto elements = self.get("elements");
            const auto value = array.get(add(i, offset).toPropertyKey());
            elements.set(std::to_string(i), value);
        }
        return self;
    }
    if (name == "toArray") {
        if (array.isUndefined()) array = JSValue::array();
        const auto elements = self.get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto key = i == 0 ? offset : add(offset, i);
            const auto value = elements.get(std::to_string(i)); array.set(key, value);
        }
        return array;
    }
    throw std::logic_error("Unknown managed matrix method");
}
JSValue constructor(unsigned width) {
    static thread_local JSWeakValue weak[2]; static thread_local bool initializing[2]{};
    const auto index = width - 3;
    if (const auto ctor = weak[index].lock(); ctor && ctor->isConstructible()) return *ctor;
    const auto name = width == 3 ? "Matrix3" : "Matrix4";
    JSModuleInitializationScope scope(initializing[index], name);
    auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
        auto self = receiver; const auto width = static_cast<unsigned>(captures[0].number()), count = width * width;
        auto elements = JSValue::array(count); const auto values = identity(width);
        for (unsigned i = 0; i < count; ++i) {
            JSPropertyDescriptor d; d.value = values[i]; d.writable = d.enumerable = d.configurable = true;
            elements.defineProperty(std::to_string(i), d);
        }
        self.set("elements", elements);
        if (!arg(args, 0).isUndefined()) {
            const auto fn = self.get("set"); Args values;
            for (unsigned i = 0; i < count; ++i) values.push_back(arg(args, i));
            fn.call(self, values);
        }
        return {};
    }, {width}, name, width * width);
    auto prototype = ctor.get("prototype");
    // Preserve source order among the implemented class methods.
    const auto methods = width == 3
        ? std::vector<std::pair<const char*, unsigned>>{{"set",9},{"identity",0},{"copy",1},{"equals",1},{"fromArray",1},{"toArray",0},{"clone",0}}
        : std::vector<std::pair<const char*, unsigned>>{{"set",16},{"identity",0},{"clone",0},{"copy",1},{"equals",1},{"fromArray",1},{"toArray",0}};
    for (const auto& [key, arity] : methods) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {key,width}, key, arity);
        d.writable = d.configurable = true; prototype.defineProperty(key, d);
    }
    prototype.set(std::string("is") + name, true);
    ctor.retainForHeapLifetime(); weak[index] = JSWeakValue(ctor); return ctor;
}
}
JSValue nativeMatrix3Constructor() { return constructor(3); }
JSValue nativeMatrix4Constructor() { return constructor(4); }
