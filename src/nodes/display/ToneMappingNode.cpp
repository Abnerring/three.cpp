#include "nodes/display/ToneMappingNode.h"

#include "nodes/core/NodeUtils.h"
#include <array>

double ToneMapping::customCacheKey(int toneMapping) {
    return hash(std::array<double,1>{static_cast<double>(toneMapping)});
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
