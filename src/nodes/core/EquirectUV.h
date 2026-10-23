#pragma once

// Port of the two functions in src/nodes/utils/EquirectUV.js at the pinned revision, which take
// a direction to texture coordinates and back. The two are not written the same way: only one of
// the coordinates needs a clamp before it is worked out, and the horizontal one asks for an
// angle with its two parts in an order worth noticing.
class EquirectUV {
  public:
    // The coordinates a direction maps to. The horizontal one comes from the angle whose parts
    // are the third and first of the direction, in that order, so the third is passed first;
    // swapping them would turn the whole texture half way round. The vertical one comes from
    // the second part, clamped to the range the angle's inverse accepts first.
    struct Coordinates {
        double u = 0, v = 0;
    };

    static Coordinates toCoordinates(double x, double y, double z);

    // The direction a pair of coordinates maps back to. It does not clamp, since the angles it
    // works out from are inside their range for any pair the first function produced.
    struct Direction {
        double x = 0, y = 0, z = 0;
    };

    static Direction toDirection(double u, double v);

    // The clamp the vertical coordinate applies before its angle, which accepts only the range
    // minus one to one and answers with a not a number outside it.
    static double clampForAngle(double value);
};
