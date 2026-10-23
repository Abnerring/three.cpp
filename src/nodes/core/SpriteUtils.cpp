#include "nodes/core/SpriteUtils.h"

SpriteUtils::Branch SpriteUtils::branch(bool rotationFlagIsDefined) {
    // Whether the flag is there, not what it says.
    return rotationFlagIsDefined ? Branch::Rotation : Branch::Axes;
}

bool SpriteUtils::flagHasEffect(bool flagIsDefined) {
    // No flag has any effect, since each is defined by its default.
    return !flagIsDefined;
}

bool SpriteUtils::unreachableBranchSetsBothAxes(bool horizontalDefined, bool verticalDefined) {
    // Each axis is set when its own flag is defined, which both are by default.
    return horizontalDefined && verticalDefined;
}

SpriteUtils::Axis SpriteUtils::rightFromLook(double lookX, double lookZ) {
    // The horizontal part becomes the third and the negated one the first.
    Axis out;
    out.x = lookZ;
    out.y = 0.0;
    out.z = -lookX;
    return out;
}

double SpriteUtils::centreComponent(double worldPosition, double rotatedLocalPosition) {
    // The world position less the rotated local one.
    return worldPosition - rotatedLocalPosition;
}
