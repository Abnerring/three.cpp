#pragma once
#include "core/JSValue.h"
#include <cmath>
#include <optional>

namespace JSOperators {
inline std::vector<std::uint16_t> units(const std::string& value) {
    std::vector<std::uint16_t> result;
    for (std::size_t i = 0; i < value.size();) {
        auto first = static_cast<unsigned char>(value[i++]); std::uint32_t c = first; unsigned tail = 0;
        if (first >= 0xf0) { c &= 7; tail = 3; } else if (first >= 0xe0) { c &= 15; tail = 2; }
        else if (first >= 0xc0) { c &= 31; tail = 1; }
        while (tail-- && i < value.size()) c = (c << 6) | (static_cast<unsigned char>(value[i++]) & 63);
        if (c > 0xffff) { c -= 0x10000; result.push_back(static_cast<std::uint16_t>(0xd800 + (c >> 10))); result.push_back(static_cast<std::uint16_t>(0xdc00 + (c & 1023))); }
        else result.push_back(static_cast<std::uint16_t>(c));
    }
    return result;
}
inline std::optional<int> compare(JSValue left, JSValue right) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number);
    const auto b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) {
        const auto av = units(a.string()), bv = units(b.string()); return av < bv ? -1 : av > bv ? 1 : 0;
    }
    const auto x = a.toNumber(), y = b.toNumber();
    if (std::isnan(x) || std::isnan(y)) return {};
    return x < y ? -1 : x > y ? 1 : 0;
}
inline bool less(JSValue a, JSValue b) { const auto c = compare(a,b); return c && *c < 0; }
inline bool greater(JSValue a, JSValue b) { const auto c = compare(a,b); return c && *c > 0; }
inline bool lessEqual(JSValue a, JSValue b) { const auto c = compare(a,b); return c && *c <= 0; }
inline bool greaterEqual(JSValue a, JSValue b) { const auto c = compare(a,b); return c && *c >= 0; }
inline std::int32_t int32(JSValue value) {
    const auto n = value.toNumber(); if (!std::isfinite(n) || n == 0) return 0;
    double mod = std::fmod(std::trunc(n),4294967296.0); if (mod < 0) mod += 4294967296.0;
    return static_cast<std::int32_t>(mod >= 2147483648.0 ? mod - 4294967296.0 : mod);
}
}
