#include "loaders/EXRLoader.h"

#include <cmath>
#include <cstring>

int EXRLoader::hufLength(int code) {
    // The low six bits.
    return code & 63;
}

int EXRLoader::hufCode(int code) {
    // Everything above them.
    return code >> 6;
}

bool EXRLoader::hufEntryValid(int code) {
    const int length = hufLength(code);
    // No code can be empty.
    if (length == 0) return false;
    // A code that reaches past its own length cannot be encoded.
    return (hufCode(code) >> length) == 0;
}

const std::array<int, 64>& EXRLoader::unZigZagOrder() {
    // The order the source spells out, one line per ten entries.
    static const std::array<int, 64> order{
        0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,  13, 16, 26, 29, 42,
        3,  8,  12, 17, 25, 30, 41, 43, 9,  11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63,
    };
    return order;
}

int EXRLoader::unZigZagIndex(int k) {
    // An index outside the block reads nothing; the caller is expected to stay in range but this
    // keeps a bad file from reaching past the table.
    if (k < 0 || k >= 64) return -1;
    return unZigZagOrder()[static_cast<std::size_t>(k)];
}

int EXRLoader::uint16Bits(int value) {
    // The low sixteen bits.
    return value & 0xFFFF;
}

int EXRLoader::int16Bits(int value) {
    const int bits = uint16Bits(value);
    // Above the signed maximum the value is negative, a whole range lower.
    return bits > 0x7FFF ? bits - 0x10000 : bits;
}

EXRLoader::WaveletPair EXRLoader::wdec14(int l, int h) {
    const int low = int16Bits(l);
    const int high = int16Bits(h);
    // Half of the high coefficient, rounded up by its parity, recovers the first sample.
    const int sum = low + (high & 1) + (high >> 1);
    WaveletPair pair;
    pair.a = sum;
    pair.b = sum - high;
    return pair;
}

void EXRLoader::csc709Inverse(YcbcrPixel& pixel) {
    // The three inputs are read before any output is written, which matters because they share
    // the same storage.
    const double y = pixel.y;
    const double cb = pixel.cb;
    const double cr = pixel.cr;
    pixel.y = y + 1.5747 * cr;
    pixel.cb = y - 0.1873 * cb - 0.4682 * cr;
    pixel.cr = y + 1.8556 * cb;
}

void EXRLoader::csc709InverseBlock(YcbcrPixel* pixels, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) csc709Inverse(pixels[i]);
}

std::uint16_t EXRLoader::floatToHalfBits(float value) {
    // The bits of the number, read without changing them.
    std::uint32_t bits = 0;
    static_assert(sizeof(float) == sizeof(bits), "a float must be four bytes here");
    std::memcpy(&bits, &value, sizeof(bits));

    const std::uint32_t sign = (bits >> 16) & 0x8000u;
    const std::uint32_t magnitude = bits & 0x7fffffffu;

    // Too large for a half: infinity, or not a number if it already was one.
    if (magnitude >= 0x47800000u) {
        return static_cast<std::uint16_t>(sign | (magnitude > 0x7f800000u ? kHalfNotANumber
                                                                        : kHalfInfinity));
    }

    // Too small for a normal half. Half a step is added first, which puts the value on the grid
    // the subnormal patterns share, and the bias is then removed.
    if (magnitude < 0x38800000u) {
        const float nudged = std::fabs(value) + 0.5f;
        std::uint32_t nudgedBits = 0;
        std::memcpy(&nudgedBits, &nudged, sizeof(nudgedBits));
        return static_cast<std::uint16_t>(sign | (nudgedBits - 0x3f000000u));
    }

    // Round to nearest, with a tie going to the even pattern.
    const std::uint32_t rounded = magnitude + 0xfffu + ((magnitude >> 13) & 1u);
    return static_cast<std::uint16_t>(sign | ((rounded - 0x38000000u) >> 13));
}