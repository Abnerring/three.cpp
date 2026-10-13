#include "core/JSNumber.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
struct NumberData {
    double value;
};
const JSValue& argument(const Args& args, std::size_t index) {
    static const JSValue undefined;
    return index < args.size() ? args[index] : undefined;
}
JSPropertyDescriptor data(JSValue value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value);
    descriptor.writable = descriptor.configurable = true;
    return descriptor;
}
double integerOrInfinity(const JSValue& value) {
    const double number = value.toNumber();
    return std::isnan(number) || number == 0 ? 0 : std::trunc(number);
}
// The largest fixed result has 121 decimal digits. Base-2^32 limbs retain the
// exact binary64 value while multiplying by 5^fractionDigits and shifting by
// its binary exponent. No floating-point decimal rounding is involved.
class Integer {
    std::vector<std::uint32_t> limbs_;
    void normalize() {
        while (!limbs_.empty() && limbs_.back() == 0)
            limbs_.pop_back();
    }

  public:
    explicit Integer(std::uint64_t value) {
        if (value)
            limbs_.push_back(static_cast<std::uint32_t>(value));
        if (value >> 32)
            limbs_.push_back(static_cast<std::uint32_t>(value >> 32));
    }
    void multiplyFive() {
        std::uint64_t carry = 0;
        for (auto& limb : limbs_) {
            const auto value = std::uint64_t(limb) * 5 + carry;
            limb = static_cast<std::uint32_t>(value);
            carry = value >> 32;
        }
        if (carry)
            limbs_.push_back(static_cast<std::uint32_t>(carry));
    }
    void shiftLeft(unsigned count) {
        if (limbs_.empty())
            return;
        const unsigned whole = count / 32, partial = count % 32;
        if (partial) {
            std::uint64_t carry = 0;
            for (auto& limb : limbs_) {
                const auto value = (std::uint64_t(limb) << partial) | carry;
                limb = static_cast<std::uint32_t>(value);
                carry = value >> 32;
            }
            if (carry)
                limbs_.push_back(static_cast<std::uint32_t>(carry));
        }
        limbs_.insert(limbs_.begin(), whole, 0);
    }
    void shiftRightAndRound(unsigned count) {
        // For ties the specification chooses the larger integer. Thus the
        // highest discarded bit alone decides whether to round upward.
        const unsigned half = count - 1;
        const bool upward =
            half / 32 < limbs_.size() && ((limbs_[half / 32] >> (half % 32)) & 1u) != 0;
        const unsigned whole = count / 32, partial = count % 32;
        if (whole >= limbs_.size())
            limbs_.clear();
        else {
            limbs_.erase(limbs_.begin(), limbs_.begin() + whole);
            if (partial) {
                std::uint32_t carry = 0;
                for (std::size_t i = limbs_.size(); i-- > 0;) {
                    const auto current = limbs_[i];
                    limbs_[i] = (current >> partial) | carry;
                    carry = current << (32 - partial);
                }
            }
            normalize();
        }
        if (upward) {
            std::uint64_t carry = 1;
            for (auto& limb : limbs_) {
                const auto value = std::uint64_t(limb) + carry;
                limb = static_cast<std::uint32_t>(value);
                carry = value >> 32;
                if (!carry)
                    break;
            }
            if (carry)
                limbs_.push_back(1);
        }
    }
    std::string decimal() {
        if (limbs_.empty())
            return "0";
        std::vector<std::uint32_t> parts;
        while (!limbs_.empty()) {
            std::uint64_t remainder = 0;
            for (std::size_t i = limbs_.size(); i-- > 0;) {
                const auto value = (remainder << 32) | limbs_[i];
                limbs_[i] = static_cast<std::uint32_t>(value / 1000000000u);
                remainder = value % 1000000000u;
            }
            parts.push_back(static_cast<std::uint32_t>(remainder));
            normalize();
        }
        std::string result = std::to_string(parts.back());
        for (std::size_t i = parts.size() - 1; i-- > 0;) {
            const auto part = std::to_string(parts[i]);
            result += std::string(9 - part.size(), '0') + part;
        }
        return result;
    }
};
std::string fixedMagnitude(double value, unsigned digits) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    const auto exponent = static_cast<unsigned>((bits >> 52) & 0x7ffu);
    const auto significand = (bits & 0xfffffffffffffu) | (exponent ? 0x10000000000000u : 0u);
    const int shift = (exponent ? int(exponent) - 1023 - 52 : -1074) + int(digits);
    Integer integer(significand);
    for (unsigned i = 0; i < digits; ++i)
        integer.multiplyFive();
    if (shift < 0)
        integer.shiftRightAndRound(static_cast<unsigned>(-shift));
    else
        integer.shiftLeft(static_cast<unsigned>(shift));
    auto result = integer.decimal();
    if (digits) {
        if (result.size() <= digits)
            result.insert(0, digits + 1 - result.size(), '0');
        result.insert(result.size() - digits, 1, '.');
    }
    return result;
}
JSValue valueOf(const JSValue& self, const Args&, const Args&) { return nativeNumberValue(self); }
JSValue toString(const JSValue& self, const Args& args, const Args&) {
    const auto value = nativeNumberValue(self);
    const auto& input = argument(args, 0);
    const double radix = input.isUndefined() ? 10 : integerOrInfinity(input);
    if (radix < 2 || radix > 36)
        throw JSRangeError("Number radix must be between 2 and 36");
    if (!std::isfinite(value.number()))
        return value.toString();
    if (radix != 10)
        throw JSNumberRadixUnsupported();
    return value.toString();
}
JSValue toFixed(const JSValue& self, const Args& args, const Args&) {
    const double value = nativeNumberValue(self).number();
    const double digits = integerOrInfinity(argument(args, 0));
    if (!std::isfinite(digits) || digits < 0 || digits > 100)
        throw JSRangeError("Number fraction digits must be between 0 and 100");
    if (!std::isfinite(value))
        return JSValue(value).toString();
    const bool negative = value < 0;
    const double magnitude = negative ? -value : value;
    const auto result = magnitude >= 1e21
                            ? JSValue(magnitude).toString()
                            : fixedMagnitude(magnitude, static_cast<unsigned>(digits));
    return negative ? "-" + result : result;
}
JSValue allocate(const JSValue& number) {
    if (number.type() != JSValue::Type::Number)
        throw JSTypeError("NumberData requires a primitive number");
    auto payload = std::make_shared<NumberData>(NumberData{number.number()});
    return JSValue::hostObject(payload, payload.get(), "three.Number");
}
} // namespace
bool isNativeNumberObject(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
    return value.isHostObject("three.Number");
}
JSValue nativeNumberValue(const JSValue& receiver) {
    static_cast<void>(JSWeakValue(receiver));
    if (receiver.type() == JSValue::Type::Number)
        return receiver;
    if (receiver.isHostObject("three.Number"))
        return std::static_pointer_cast<NumberData>(receiver.hostPayload("three.Number"))->value;
    throw JSTypeError("Number method called on incompatible receiver");
}
JSValue nativeNumberPrototype() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject())
        return *value;
    auto prototype = allocate(0);
    const auto install = [&](const char* name, JSValue::NativeFunction callback, unsigned arity) {
        auto method = JSValue::function(callback, {}, name, arity);
        // Intrinsic methods survive deletion of their public prototype field,
        // following the same per-heap policy as Object and Array built-ins.
        method.retainForHeapLifetime();
        prototype.defineProperty(name, data(method));
    };
    install("toFixed", toFixed, 1);
    install("toString", toString, 1);
    install("valueOf", valueOf, 0);
    prototype.retainForHeapLifetime();
    cached = JSWeakValue(prototype);
    return prototype;
}
JSValue createNativeNumberObject(const JSValue& primitiveNumber) {
    static_cast<void>(JSWeakValue(primitiveNumber));
    auto result = allocate(primitiveNumber);
    result.setPrototype(nativeNumberPrototype());
    return result;
}
