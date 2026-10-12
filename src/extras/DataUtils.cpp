#include "extras/DataUtils.h"
#include "math/MathUtils.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>

namespace {
// Same conversion tables and truncation as three.js extras/DataUtils.js.
struct Tables {
    std::array<std::uint32_t, 512> base{}, shift{};
    std::array<std::uint32_t, 2048> mantissa{};
    std::array<std::uint32_t, 64> exponent{}, offset{};
    constexpr Tables() {
        for (int i = 0; i < 256; ++i) {
            const int e = i - 127;
            if (e < -27) {
                base[i] = 0; base[i | 0x100] = 0x8000;
                shift[i] = shift[i | 0x100] = 24;
            } else if (e < -14) {
                base[i] = 0x0400 >> (-e - 14);
                base[i | 0x100] = base[i] | 0x8000;
                shift[i] = shift[i | 0x100] = -e - 1;
            } else if (e <= 15) {
                base[i] = (e + 15) << 10;
                base[i | 0x100] = base[i] | 0x8000;
                shift[i] = shift[i | 0x100] = 13;
            } else {
                base[i] = 0x7c00; base[i | 0x100] = 0xfc00;
                shift[i] = shift[i | 0x100] = e < 128 ? 24 : 13;
            }
        }
        for (std::uint32_t i = 1; i < 1024; ++i) {
            std::uint32_t m = i << 13, e = 0;
            while ((m & 0x00800000) == 0) { m <<= 1; e -= 0x00800000; }
            m &= ~0x00800000u;
            e += 0x38800000;
            mantissa[i] = m | e;
        }
        for (std::uint32_t i = 1024; i < 2048; ++i)
            mantissa[i] = 0x38000000 + ((i - 1024) << 13);
        for (std::uint32_t i = 1; i < 31; ++i) exponent[i] = i << 23;
        exponent[31] = 0x47800000; exponent[32] = 0x80000000;
        for (std::uint32_t i = 33; i < 63; ++i)
            exponent[i] = 0x80000000 + ((i - 32) << 23);
        exponent[63] = 0xc7800000;
        for (std::size_t i = 1; i < 64; ++i) if (i != 32) offset[i] = 1024;
    }
};
constexpr Tables tables;
}

std::uint16_t toHalfFloat(double value) {
    if (std::abs(value) > 65504)
        std::cerr << "THREE.DataUtils.toHalfFloat(): Value out of range.\n";
    // Math.min/max canonicalize NaN to this Float32 pattern in the pinned V8
    // oracle. Its sign is observable here because the return value is raw bits.
    const auto bits = std::isnan(value) ? 0xffc00000u :
        std::bit_cast<std::uint32_t>(static_cast<float>(MathUtils::clamp(value, -65504, 65504)));
    const auto e = (bits >> 23) & 0x1ff;
    return static_cast<std::uint16_t>(tables.base[e] + ((bits & 0x007fffff) >> tables.shift[e]));
}

double fromHalfFloat(double value) {
    // Bitwise operators apply ToInt32, including modulo wrapping and non-finite -> 0.
    double wrapped = std::isfinite(value) ? std::fmod(std::trunc(value), 4294967296.0) : 0;
    if (wrapped < 0) wrapped += 4294967296.0;
    const auto bits = static_cast<std::uint32_t>(wrapped);
    // Missing table entries produce NaN, then Uint32Array stores zero upstream.
    if (bits > 65535) return 0;
    const auto m = bits >> 10;
    return std::bit_cast<float>(tables.mantissa[tables.offset[m] + (bits & 0x3ff)] + tables.exponent[m]);
}
