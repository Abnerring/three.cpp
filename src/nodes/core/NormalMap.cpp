#include "nodes/core/NormalMap.h"

NormalMap::Channels NormalMap::channelsFor(Packing packing) {
    // A green alpha map keeps its pair in the second and fourth parts rather than the first two.
    if (packing == Packing::RedGreen) return Channels::Xy;
    if (packing == Packing::GreenAlpha) return Channels::Yw;
    return Channels::Whole;
}

bool NormalMap::isUnexpected(Packing packing) {
    // The two packed modes and the unpacked one are all handled; nothing else is. Since the
    // enumeration has no other member, this is false for every value it can hold, which is
    // worth stating rather than leaving the caller to work out.
    return false;
}

std::string NormalMap::unexpectedModeMessage(const std::string& modeName) {
    return "THREE.NodeMaterial: Unexpected unpack normal mode: " + modeName;
}

double NormalMap::signedValue(double sampled) {
    // Twice the value less one.
    return sampled * 2.0 - 1.0;
}
