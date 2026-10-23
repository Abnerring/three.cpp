#pragma once

// Port of the two functions in src/nodes/utils/Remap.js at the pinned revision. The remapping
// itself is the usual two step affine map; what is worth writing down is the test that decides
// whether to clamp, which asks whether the argument is there rather than what it says.
class Remap {
  public:
    // The remapped value, before any clamp. The input range is normalised to nought and one and
    // then stretched and shifted into the output range; a zero width input range divides by
    // nought and gives an infinity rather than being guarded.
    static double value(double node, double inLow, double inHigh, double outLow, double outHigh);

    // The same with the normalised value clamped to the range first. The two differ only for an
    // input outside its own range.
    static double clamped(double node, double inLow, double inHigh, double outLow, double outHigh);

    // Whether the clamp step runs. The test asks whether the argument is defined rather than
    // whether it is true, and the argument has a default, so it is always defined: even the
    // default false counts, which means the clamp step runs for a call that never mentioned it.
    // That is what the source does, and the two functions it exports therefore differ in nothing
    // but their names.
    static bool runsClampStep(bool argumentIsDefined);
};
