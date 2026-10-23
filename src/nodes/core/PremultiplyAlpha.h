#pragma once
#include <cmath>

// Port of the two functions in src/nodes/display/PremultiplyAlphaFunctions.js at the pinned
// revision. One multiplies a colour's parts by its alpha and the other divides them by it, and
// the two are not mirror images: only the division needs a guard, because only a division by
// nought is a problem.
class PremultiplyAlpha {
  public:
    // A colour in four parts, with the alpha last.
    struct Colour {
        double r = 0, g = 0, b = 0, a = 0;
    };

    // Multiplies the three parts by the alpha and keeps the alpha. An alpha of nought gives a
    // colour whose three parts are nought, which needs no special case.
    static Colour premultiply(const Colour& colour);

    // Divides the three parts by the alpha and keeps the alpha. An alpha of nought would divide
    // by nought, so that case answers with four noughts rather than with an infinity or a not a
    // number; the alpha of that answer is nought as well, not the original nought kept.
    static Colour unpremultiply(const Colour& colour);

    // Whether an alpha needs the guard, which is true only for exactly nought.
    static bool needsGuard(double alpha);
};
