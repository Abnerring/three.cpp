#include "math/Matrix2.h"
#include <limits>
#include "core/JSModuleObject.h"

Matrix2::Matrix2(double n11, double n12, double n21, double n22) {
    set(n11, n12, n21, n22);
}
std::span<double, 4> Matrix2::elements() {
    return elements_;
}
std::span<const double, 4> Matrix2::elements() const {
    return elements_;
}
Matrix2& Matrix2::identity() {
    return set(1, 0, 0, 1);
}
Matrix2& Matrix2::set(double n11, double n12, double n21, double n22) {
    elements_[0] = n11;
    elements_[2] = n12;
    elements_[1] = n21;
    elements_[3] = n22;
    return *this;
}
Matrix2& Matrix2::fromArray(const std::vector<double>& array, std::size_t offset) {
    for (std::size_t i = 0; i < 4; ++i)
        elements_[i] = offset < array.size() && i < array.size() - offset
                           ? array[offset + i]
                           : std::numeric_limits<double>::quiet_NaN();
    return *this;
}

namespace {
using Args = std::vector<JSValue>;
JSValue matrixArg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue matrixAdd(const JSValue& a, const JSValue& b) {
    const auto left = a.toPrimitive(); const auto right = b.toPrimitive();
    if (left.type() == JSValue::Type::String || right.type() == JSValue::Type::String)
        return left.toString() + right.toString();
    return left.toNumber() + right.toNumber();
}
JSValue matrix2Method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    if (name == "identity") {
        const auto fn = self.get("set"); fn.call(self, {1, 0, 0, 1}); return self;
    }
    if (name == "set") {
        auto elements = self.get("elements");
        elements.set("0", matrixArg(args, 0)); elements.set("2", matrixArg(args, 1));
        elements.set("1", matrixArg(args, 2)); elements.set("3", matrixArg(args, 3)); return self;
    }
    const auto array = matrixArg(args, 0), raw = matrixArg(args, 1);
    const auto offset = raw.isUndefined() ? JSValue(0) : raw;
    for (int i = 0; i < 4; ++i) {
        // Evaluate the assignment receiver before the RHS, on every iteration.
        auto elements = self.get("elements");
        const auto value = array.get(matrixAdd(i, offset).toPropertyKey());
        elements.set(std::to_string(i), value);
    }
    return self;
}
}
JSValue nativeMatrix2Constructor() {
    static thread_local JSWeakValue weak;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "Matrix2");
    auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args&) -> JSValue {
        auto self = receiver, elements = JSValue::array(4);
        for (int i = 0; i < 4; ++i) {
            JSPropertyDescriptor d; d.value = i == 0 || i == 3 ? 1 : 0;
            d.writable = d.enumerable = d.configurable = true;
            elements.defineProperty(std::to_string(i), d);
        }
        self.set("elements", elements);
        if (!matrixArg(args, 0).isUndefined()) {
            const auto method = self.get("set");
            method.call(self, {matrixArg(args,0),matrixArg(args,1),matrixArg(args,2),matrixArg(args,3)});
        }
        return {};
    }, {}, "Matrix2", 4);
    auto prototype = ctor.get("prototype");
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*,unsigned>>{{"identity",0},{"fromArray",1},{"set",4}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(matrix2Method, {name}, name, arity);
        d.writable = d.configurable = true; prototype.defineProperty(name, d);
    }
    // Class methods are installed before execution of the static block.
    prototype.set("isMatrix2", true);
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
