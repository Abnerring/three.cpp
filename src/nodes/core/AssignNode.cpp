#include "nodes/core/AssignNode.h"

std::string AssignNode::nodeType(const std::string& output, const std::string& targetType) {
    // The empty type when nothing was asked for, the target's otherwise.
    if (output == "void") return "void";
    return targetType;
}

bool AssignNode::alreadyAssigned(bool hasPropertyName) {
    return hasPropertyName;
}

bool AssignNode::needsSplit(bool swizzleAssignAvailable, bool targetIsSplit, int componentCount,
                            int targetLength, const std::string& components) {
    // All three first conditions must hold.
    if (swizzleAssignAvailable) return false;
    if (!targetIsSplit) return false;
    if (componentCount <= 1) return false;
    // And the components must differ from the first ones in order, so naming them the identity
    // way needs no split while naming them any other way does.
    const std::string identity = std::string("xyzw").substr(0, static_cast<std::size_t>(targetLength));
    return components != identity;
}

bool AssignNode::addsLineToFlow(const std::string& output, const std::string& sourceType) {
    // Only for a void output or a void source.
    return output == "void" || sourceType == "void";
}
