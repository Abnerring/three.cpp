#include "nodes/core/RotateNode.h"

#include <cmath>

RotateNode::Pair RotateNode::rotate2D(const Pair& point, double radians) {
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    // The matrix has columns (c, s) and (-s, c); multiplying a column vector by it gives this.
    Pair out;
    out.x = c * point.x + (-s) * point.y;
    out.y = s * point.x + c * point.y;
    return out;
}

bool RotateNode::usesSharedSineOrder(char axis) {
    // The first and third axes put the sine where the second puts its negation.
    return axis != 'Y';
}

RotateNode::Column RotateNode::firstColumn(char axis, double radians) {
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    Column out;
    if (axis == 'X') {
        // The first column of the turn about the first axis is untouched.
        out.a = 1.0; out.b = 0.0; out.c = 0.0; out.d = 0.0;
    } else if (axis == 'Y') {
        // Here the sine is negated in the first column and positive in the third, which is the
        // opposite of the other two.
        out.a = c; out.b = 0.0; out.c = -s; out.d = 0.0;
    } else {
        // The turn about the third axis puts the cosine and sine first.
        out.a = c; out.b = s; out.c = 0.0; out.d = 0.0;
    }
    return out;
}

bool RotateNode::orderIsLeftToRight() {
    // The first character names the leftmost factor.
    return true;
}

bool RotateNode::cacheKeyIncludesOrder() {
    // Two nodes differing only in order build different shaders, so the key must differ.
    return true;
}
