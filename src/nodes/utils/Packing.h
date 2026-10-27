#pragma once
#include <string>

// Port of the parts of src/nodes/utils/Packing.js at the pinned revision that can be worked out
// without a shader builder: the two colour conversions, the hemisphere reconstruction and the
// two renamed helpers.
class Packing {
  public:
    // A direction's part taken into the range nought to one, which is half of it plus a half.
    static double packPart(double direction);

    // The same part taken back into the range minus one to one.
    static double unpackPart(double colour);

    // The clamp the reconstruction applies to what is left of the hemisphere. It is a clamp and
    // not an absolute value: a part of the direction that lies outside the unit circle leaves a
    // negative remainder, and the clamp answers nought where an absolute value would answer the
    // size of the overshoot and so invent a normal that was never there.
    static double saturate(double value);

    // The third part of a reconstructed normal, given the first two. The remainder of one less
    // the squared length is clamped before the square root, so a pair outside the unit circle
    // gives nought rather than a not a number.
    static double reconstructedZ(double x, double y);

    // The three sentences the renamed helpers print, each naming both its names.
    static std::string packRenameWarning();
    static std::string unpackRenameWarning();
};

#include "core/JSValue.h"
JSValue nativePackingModule();
JSValue nativePackingBinding(const std::string& name);
