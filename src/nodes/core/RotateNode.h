#pragma once
#include <string>

// Port of the matrix construction in src/nodes/utils/RotateNode.js at the pinned revision. The
// two dimensional case is one matrix and the three dimensional one is three chained; what is
// worth writing down is the sign that two of the three share and the third does not.
class RotateNode {
  public:
    static constexpr const char* kTypeName = "RotateNode";

    // A two dimensional point turned by an angle, counter clockwise. The matrix is built from
    // the cosine and sine in a fixed pattern whose first column is the cosine and sine and
    // second is the negated sine and cosine, which is the standard form.
    struct Pair {
        double x = 0, y = 0;
    };

    static Pair rotate2D(const Pair& point, double radians);

    // One column of a three dimensional rotation matrix, given the axis and the angle. The
    // first and third axes build their columns from the cosine and sine in one order and the
    // second axis the other way round, which is the sign that separates it from the other two.
    struct Column {
        double a = 0, b = 0, c = 0, d = 0;
    };

    static Column firstColumn(char axis, double radians);

    // Whether the axis builds its columns with the sine sign the other two use, which is true
    // for the first and third and false for the second.
    static bool usesSharedSineOrder(char axis);

    // The order the three matrices are multiplied in, as the index of each axis in the string.
    // The multiplication runs from the first character to the last, so the first named axis is
    // the leftmost factor.
    static bool orderIsLeftToRight();

    // The caching name for an order, which must include it since two nodes differing only in
    // order build different shaders.
    static bool cacheKeyIncludesOrder();
};
