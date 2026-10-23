#include "nodes/core/ViewportUtils.h"

ViewportUtils::Choice ViewportUtils::choose(double depthDifference) {
    // Less than nought is the case the function corrects, and there it answers with the screen
    // coordinate rather than the one it was given.
    if (depthDifference < 0.0) return Choice::Screen;
    return Choice::Given;
}
