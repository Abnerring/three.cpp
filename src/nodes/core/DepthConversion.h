#pragma once

// Port of the four depth conversions in src/nodes/display/ViewportDepthNode.js at the pinned
// revision. Each takes a view space depth, which is negative in front of the camera, and answers
// a depth in the range nought to one. The reversed pair are the same curves run the other way,
// which shows up as a sign that flips between the two.
class DepthConversion {
  public:
    // A view space depth to an orthographic depth. The divisor is the near less the far, which
    // is negative since the far is the larger, so the quotient rises from nought at the near
    // plane to one at the far.
    static double viewZToOrthographicDepth(double viewZ, double near, double far);

    // The same to a reversed orthographic depth, which is one at the near plane and nought at
    // the far. The divisor here is the far less the near, the opposite sign of the one above,
    // and that difference is the whole of what reverses the curve.
    static double viewZToReversedOrthographicDepth(double viewZ, double near, double far);

    // A depth back to a view space depth. There are two forms because the renderer may keep its
    // depth buffer reversed, and the choice is made by a strict comparison against true.
    static double orthographicDepthToViewZ(double depth, double near, double far, bool reversed);

    // A view space depth to a perspective depth. The numerator adds the near value to the view
    // depth rather than taking it away, since the view depth is negative; at the near plane that
    // sum is nought and the answer is nought, and the denominator is nought only at the camera
    // itself.
    static double viewZToPerspectiveDepth(double viewZ, double near, double far);
};
