#include "nodes/core/FrontFacing.h"

FrontFacing::Answer FrontFacing::answer(bool fragmentStage, Side side) {
    // Facing is a fragment stage question, so anything else says true without asking.
    if (!fragmentStage) return Answer::True;
    // A material drawn only on the back is never front facing.
    if (side == Side::Back) return Answer::False;
    return Answer::AskBuilder;
}

double FrontFacing::faceDirection(bool frontFacing) {
    // Twice the flag less one: true gives one and false gives minus one.
    const double flag = frontFacing ? 1.0 : 0.0;
    return flag * 2.0 - 1.0;
}

double FrontFacing::backSideFactor(Side side, bool frontFacing) {
    // Sided to the front leaves the vector alone; sided to the back always turns it over; sided
    // both ways turns it over exactly where the facing number is minus one.
    if (side == Side::Front) return 1.0;
    if (side == Side::Back) return -1.0;
    return faceDirection(frontFacing);
}

std::string FrontFacing::deprecatedRenameWarning() {
    return "TSL: \"directionToFaceDirection()\" has been renamed to \"negateOnBackSide()\".";
}
