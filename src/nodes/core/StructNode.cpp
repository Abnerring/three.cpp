#include "nodes/core/StructNode.h"

bool StructNode::constructorTypeIsUsed() {
    // The type method is overridden, so the constructor's argument never comes out.
    return false;
}

StructNode::Values StructNode::valuesKind(int argumentCount, bool firstArgumentIsAnArray) {
    if (argumentCount == 0) return Values::None;
    if (firstArgumentIsAnArray) return Values::ZippedByName;
    return Values::TakenAsGiven;
}

std::string StructNode::memberForPosition(const std::vector<std::string>& memberOrder,
                                          int position) {
    // The names are taken in the layout's order, so the position decides and the argument's own
    // meaning does not.
    if (position < 0 || position >= static_cast<int>(memberOrder.size())) return std::string();
    return memberOrder[static_cast<std::size_t>(position)];
}

bool StructNode::movesStructTypeLast() {
    return true;
}

bool StructNode::isCacheable() {
    return false;
}
