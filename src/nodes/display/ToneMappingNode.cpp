#include "nodes/display/ToneMappingNode.h"

namespace {
// The hash the source uses for a number, which mixes the bits so that small neighbouring
// values do not give neighbouring keys.
std::uint32_t mix(std::uint32_t value) {
    value = (value ^ 61u) ^ (value >> 16);
    value = value + (value << 3);
    value = value ^ (value >> 4);
    value = value * 0x27d4eb2du;
    value = value ^ (value >> 15);
    return value;
}
}  // namespace

std::uint32_t ToneMapping::customCacheKey(int toneMapping) {
    // The mapping is folded in, so the key changes with it.
    return mix(static_cast<std::uint32_t>(toneMapping));
}

bool ToneMapping::keysCollide(int first, int second) {
    // Two different mappings give two different keys, which is what lets the cache serve them
    // separately.
    return customCacheKey(first) == customCacheKey(second);
}

bool ToneMapping::usesGivenColour(bool hasColourNode, bool colourNodeIsTruthy) {
    // The short-circuiting or means a colour that is there but falsy also falls back.
    return hasColourNode && colourNodeIsTruthy;
}
