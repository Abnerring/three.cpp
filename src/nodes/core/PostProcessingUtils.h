#pragma once
#include <string>

// Port of the arithmetic in src/nodes/utils/PostProcessingUtils.js at the pinned revision. The
// module turns between screen, clip and view space, and two of its conversions depend on a
// renderer setting in a way that changes the arithmetic rather than only the constants.
class PostProcessingUtils {
  public:
    // Whether a depth belongs to the background, meaning the cleared far plane. The test is
    // against nought when the depth buffer is reversed and against one when it is not, so the
    // two comparisons point in opposite directions; both include their boundary, so a depth of
    // exactly one is the background in the ordinary case and exactly nought in the reversed one.
    static bool isBackgroundDepth(bool reversedDepthBuffer, double depth);

    // The clip space position built from a screen position and a depth.
    struct Clip {
        double x = 0, y = 0, z = 0, w = 1;
    };

    // The position the inverse projection is applied to. The vertical coordinate is turned over
    // in both cases, but where the depth is scaled differs: with a reversed depth buffer or the
    // newer coordinate system the depth already lies in clip space and is passed through, while
    // in the ordinary case it is taken from nought to one into minus one to one along with the
    // two screen coordinates. So the same input gives a different third part in the two cases,
    // and a test asserts both.
    static Clip clipFromScreen(bool reversedOrWebGpu, double screenX, double screenY, double depth);

    // A screen position from a clip space position: the first two parts divided by the fourth,
    // taken from minus one to one into nought to one, and then the vertical turned over. The
    // turning is the last step rather than the first, so the two orders are not interchangeable.
    struct Pair {
        double x = 0, y = 0;
    };

    static Pair screenFromClip(double clipX, double clipY, double clipW);

    // The kind of depth the module asks for when it needs one, which is the linear kind rather
    // than the raw buffer value.
    static constexpr const char* kDepthKind = "linear";
};
