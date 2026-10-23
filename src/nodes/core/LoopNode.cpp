#include "nodes/core/LoopNode.h"

std::string LoopNode::varName(int index) {
    // The letter i plus the index, character by character.
    return std::string(1, static_cast<char>('i' + index));
}

std::vector<std::string> LoopNode::internalParams(bool firstIsFunction) {
    if (firstIsFunction) {
        // A constant true in front, so the callback becomes the body of an unbounded loop.
        return {"bool(true)", "body"};
    }
    return {"descriptors", "body"};
}

int LoopNode::descriptorCount(int paramCount) {
    // Everything but the last, so a one entry list has none.
    const int count = paramCount - 1;
    return count < 0 ? 0 : count;
}

std::string LoopNode::paramName(bool paramIsNode, bool hasName, const std::string& name,
                                int index) {
    if (!paramIsNode && hasName) return name;
    return varName(index);
}

std::string LoopNode::paramType(bool paramIsNode, bool hasType, const std::string& type) {
    if (!paramIsNode && hasType) return type;
    return kDefaultType;
}

bool LoopNode::bodyIsInvokedOnce() {
    return true;
}

bool LoopNode::isCacheable() {
    return false;
}
