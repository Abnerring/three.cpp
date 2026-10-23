#include "nodes/core/SubBuildNode.h"

bool SubBuildNode::bracketsTheTypeQuery(bool typeWasGiven) {
    // A type that was given is returned before the bracket opens.
    return !typeWasGiven;
}

bool SubBuildNode::bracketsTheBuild() {
    return true;
}

bool SubBuildNode::balancedOnThrow() {
    // A plain remove after the call, with nothing to run it on the way out.
    return false;
}

bool SubBuildNode::isCacheable() {
    return false;
}
