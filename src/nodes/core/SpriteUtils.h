#pragma once

// Port of the decisions in src/nodes/utils/SpriteUtils.js at the pinned revision. The billboard
// helper takes three flags and tests each of them by asking whether it is defined, and all three
// have defaults, so all three are always defined and none of them can be turned off.
class SpriteUtils {
  public:
    // Which branch the helper takes. The test is whether the rotation flag is defined, and the
    // flag has a default, so the rotation branch is always the one taken and the other branch is
    // never reached, whatever a caller passes.
    enum class Branch { Rotation, Axes };

    static Branch branch(bool rotationFlagIsDefined);

    // Whether a flag the helper tests has any effect. Since every one of them is defined by its
    // default, none does: passing false for the vertical flag does not stop the vertical axis
    // being set, because the test is not whether the flag is true.
    static bool flagHasEffect(bool flagIsDefined);

    // Whether the two axes in the unreachable branch are set, which they would be for any
    // caller, since both their flags are defined by default.
    static bool unreachableBranchSetsBothAxes(bool horizontalDefined, bool verticalDefined);

    // The rotation about the vertical axis, built from the horizontal direction in the same
    // swap and negate pattern used by the matcap coordinates: the horizontal part becomes the
    // first and the negated part the third.
    struct Axis {
        double x = 0, y = 0, z = 0;
    };

    static Axis rightFromLook(double lookX, double lookZ);

    // The centre the billboard turns about, when none was given. It is the world position less
    // the model matrix applied to the local position as a direction, so the two are not simply
    // added; the subtraction is what the source does.
    static double centreComponent(double worldPosition, double rotatedLocalPosition);

    // The default the three flags carry, which is what makes them always defined.
    static constexpr bool kDefaultHorizontal = true;
    static constexpr bool kDefaultVertical = false;
    static constexpr bool kDefaultRotation = false;
};
