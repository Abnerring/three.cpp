#include "nodes/core/PostProcessingUtils.h"

bool PostProcessingUtils::isBackgroundDepth(bool reversedDepthBuffer, double depth) {
    // The two comparisons are opposite, and both include their boundary.
    if (reversedDepthBuffer) return depth <= 0.0;
    return depth >= 1.0;
}

PostProcessingUtils::Clip PostProcessingUtils::clipFromScreen(bool reversedOrWebGpu,
                                                            double screenX, double screenY,
                                                            double depth) {
    Clip out;
    if (reversedOrWebGpu) {
        // The two screen coordinates are taken into clip space first, and the depth is passed
        // through as it already is one.
        out.x = screenX * 2.0 - 1.0;
        out.y = (1.0 - screenY) * 2.0 - 1.0;
        out.z = depth;
    } else {
        // All three are taken into clip space together, so the depth is scaled as well.
        out.x = screenX * 2.0 - 1.0;
        out.y = (1.0 - screenY) * 2.0 - 1.0;
        out.z = depth * 2.0 - 1.0;
    }
    out.w = 1.0;
    return out;
}

PostProcessingUtils::Pair PostProcessingUtils::screenFromClip(double clipX, double clipY,
                                                            double clipW) {
    const double x = (clipX / clipW) * 0.5 + 0.5;
    const double y = (clipY / clipW) * 0.5 + 0.5;
    Pair out;
    out.x = x;
    // The turning comes after the range map, not before it.
    out.y = 1.0 - y;
    return out;
}
