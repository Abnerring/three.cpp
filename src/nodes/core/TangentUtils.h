#pragma once

// Port of the arithmetic in src/nodes/accessors/TangentUtils.js at the pinned revision, which
// works out a tangent frame from the derivatives of the view position and of the texture
// coordinate, following http://www.thetenthplanet.de/archives/1180.
class TangentUtils {
  public:
    // The scale the two frame vectors are multiplied by. The determinant is the larger of the
    // two squared lengths rather than their product with the cross term taken off, which is the
    // form the reference gives; the source uses the larger, so the port does too. A determinant
    // of nought gives a scale of nought rather than an infinity, which is what keeps a degenerate
    // surface from producing an undefined frame.
    static double scale(double tDotT, double bDotB);

    // The determinant the scale is worked out from, which is the larger of the two squared
    // lengths.
    static double determinant(double tDotT, double bDotB);

    // Whether a determinant is nought and so gets no scale at all.
    static bool degenerate(double determinant);
};
