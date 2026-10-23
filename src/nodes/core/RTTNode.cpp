#include "nodes/core/RTTNode.h"

bool RTTNode::autoResize(bool hasWidth) {
    // Resizing is on exactly when no width was given.
    return !hasWidth;
}

int RTTNode::sizeOr(int size, bool hasSize) {
    // Missing means the default; nought is kept.
    if (!hasSize) return kDefaultSize;
    return size;
}

std::string RTTNode::textureType(bool callerNamedType, const std::string& callerType) {
    // The caller's type overrides the default.
    if (callerNamedType) return callerType;
    return "HalfFloatType";
}

bool RTTNode::stateIsPerInstance() {
    return true;
}

bool RTTNode::isCacheable() {
    return false;
}
