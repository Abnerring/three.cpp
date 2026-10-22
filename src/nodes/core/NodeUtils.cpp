#include "nodes/core/NodeUtils.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace {
// JavaScript's number to int32 conversion for the values that reach the hash: truncation
// toward zero, then wrap-around into the int32 range.
std::int32_t toInt32(double value) {
    if (value != value || value == std::numeric_limits<double>::infinity() ||
        value == -std::numeric_limits<double>::infinity())
        return 0;
    const double truncated = value < 0 ? -std::floor(-value) : std::floor(value);
    const double wrapped = std::fmod(truncated, 4294967296.0);
    const double positive = wrapped < 0 ? wrapped + 4294967296.0 : wrapped;
    return positive >= 2147483648.0 ? static_cast<std::int32_t>(positive - 4294967296.0)
                                    : static_cast<std::int32_t>(positive);
}

double cyrb53Array(std::span<const double> values) {
    std::uint32_t h1 = 0xdeadbeefu;
    std::uint32_t h2 = 0x41c6ce57u;
    for (const double value : values) {
        const auto v = static_cast<std::uint32_t>(toInt32(value));
        h1 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ v), 2654435761));
        h2 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ v), 1597334677));
    }
    h1 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 16)), 2246822507));
    h1 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 13)), 3266489909));
    h2 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 16)), 2246822507));
    h2 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 13)), 3266489909));
    return 4294967296.0 * static_cast<double>(2097151 & h2) + static_cast<double>(h1);
}
}  // namespace

std::int32_t nodeImul(std::int32_t a, std::int32_t b) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(a) * static_cast<std::uint32_t>(b));
}

double hashArray(std::span<const double> values) { return cyrb53Array(values); }

double hash(std::span<const double> params) { return cyrb53Array(params); }

double hashString(const std::string& value) {
    std::uint32_t h1 = 0xdeadbeefu;
    std::uint32_t h2 = 0x41c6ce57u;
    for (const char ch : value) {
        const auto c = static_cast<std::uint32_t>(static_cast<unsigned char>(ch));
        h1 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ c), 2654435761));
        h2 = static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ c), 1597334677));
    }
    h1 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 16)), 2246822507));
    h1 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 13)), 3266489909));
    h2 = static_cast<std::uint32_t>(
        nodeImul(static_cast<std::int32_t>(h2 ^ (h2 >> 16)), 2246822507));
    h2 ^= static_cast<std::uint32_t>(nodeImul(static_cast<std::int32_t>(h1 ^ (h1 >> 13)), 3266489909));
    return 4294967296.0 * static_cast<double>(2097151 & h2) + static_cast<double>(h1);
}
