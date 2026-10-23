#include "nodes/core/MatcapUV.h"

double MatcapUV::coordinate(double dotProduct) {
    // The scale and the offset, in that order: the dot product is scaled first and the offset
    // added after.
    return dotProduct * kScale + kOffset;
}

MatcapUV::Axis MatcapUV::firstAxis(double viewDirectionX, double viewDirectionZ) {
    // The horizontal part of the view direction becomes the third part and the vertical part is
    // set to nought, while the negated horizontal part becomes the first.
    Axis axis;
    axis.x = viewDirectionZ;
    axis.y = 0.0;
    axis.z = -viewDirectionX;
    return axis;
}
