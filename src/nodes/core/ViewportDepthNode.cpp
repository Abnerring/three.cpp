#include "nodes/core/ViewportDepthNode.h"

std::vector<std::string> ViewportDepthNode::scopes() {
    return {kDepthBase, kDepth, kLinearDepth};
}

bool ViewportDepthNode::answersWithFragDepth(const std::string& scope) {
    // Only the base scope.
    return scope == kDepthBase;
}

ViewportDepthNode::Result ViewportDepthNode::setupResult(const std::string& scope, bool hasValue,
                                                          bool isPerspectiveCamera) {
    if (scope == kDepthBase) {
        // Without a value there is nothing to assign, so the answer is nothing at all.
        return hasValue ? Result::AssignedBase : Result::Nothing;
    }
    if (scope == kDepth) {
        // The value is ignored here whatever it holds.
        return isPerspectiveCamera ? Result::PerspectiveDepth : Result::OrthographicDepth;
    }
    if (scope == kLinearDepth) {
        if (hasValue) {
            return isPerspectiveCamera ? Result::LinearFromValue : Result::ValueAsItStands;
        }
        return Result::LinearFromPosition;
    }
    return Result::Nothing;
}

int ViewportDepthNode::conversionsForLinearValue(bool isPerspectiveCamera) {
    // Two for a perspective camera, none for an orthographic one.
    return isPerspectiveCamera ? 2 : 0;
}

bool ViewportDepthNode::valueIsUsed(const std::string& scope) {
    // The depth scope ignores it; the other two use it when it is there.
    return scope != kDepth;
}

bool ViewportDepthNode::isCacheable() {
    return false;
}
