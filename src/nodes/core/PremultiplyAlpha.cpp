#include "nodes/core/PremultiplyAlpha.h"

PremultiplyAlpha::Colour PremultiplyAlpha::premultiply(const Colour& colour) {
    // Multiplication by nought is nought, so no case is needed.
    Colour out;
    out.r = colour.r * colour.a;
    out.g = colour.g * colour.a;
    out.b = colour.b * colour.a;
    out.a = colour.a;
    return out;
}

bool PremultiplyAlpha::needsGuard(double alpha) {
    return alpha == 0.0;
}

PremultiplyAlpha::Colour PremultiplyAlpha::unpremultiply(const Colour& colour) {
    // The guard answers with four noughts, not with the original colour and not with its alpha
    // kept beside nought parts.
    if (needsGuard(colour.a)) {
        Colour out;
        return out;
    }
    Colour out;
    out.r = colour.r / colour.a;
    out.g = colour.g / colour.a;
    out.b = colour.b / colour.a;
    out.a = colour.a;
    return out;
}
