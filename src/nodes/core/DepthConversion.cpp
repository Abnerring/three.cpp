#include "nodes/core/DepthConversion.h"

double DepthConversion::viewZToOrthographicDepth(double viewZ, double near, double far) {
    return (viewZ + near) / (near - far);
}

double DepthConversion::viewZToReversedOrthographicDepth(double viewZ, double near, double far) {
    // The far is added and the divisor is the far less the near, so the curve is the one above
    // with its two ends swapped.
    return (viewZ + far) / (far - near);
}

double DepthConversion::orthographicDepthToViewZ(double depth, double near, double far,
                                                bool reversed) {
    if (reversed) return (far - near) * depth - far;
    return (near - far) * depth - near;
}

double DepthConversion::viewZToPerspectiveDepth(double viewZ, double near, double far) {
    return ((near + viewZ) * far) / ((far - near) * viewZ);
}
