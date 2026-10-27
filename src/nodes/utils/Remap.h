#pragma once
#include "core/JSValue.h"
JSValue nativeRemapModule();
JSValue nativeRemapBinding(const std::string& name);

// Port of the two functions in src/nodes/utils/Remap.js at the pinned revision. The remapping
// itself is the usual two step affine map. Clamping follows the value returned by
// TSL defined(), including false for the default bool(false) node.
class Remap {
  public:
    // The remapped value, before any clamp. The input range is normalised to nought and one and
    // then stretched and shifted into the output range; a zero width input range divides by
    // nought and gives an infinity rather than being guarded.
    static double value(double node, double inLow, double inHigh, double outLow, double outHigh);

    // The same with the normalised value clamped to the range first. The two differ only for an
    // input outside its own range.
    static double clamped(double node, double inLow, double inHigh, double outLow, double outHigh);

    // Compatibility helper for the result of TSL defined(): defined(bool(false))
    // is false, while defined(bool(true)) is true. Presence alone does not clamp.
    static bool runsClampStep(bool definedResult);
};
