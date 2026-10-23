#include "nodes/core/StackNode.h"

std::string StackNode::elementType(bool hasOutput, const std::string& outputElementType) {
    return hasOutput ? outputElementType : std::string("void");
}

std::string StackNode::nodeType(bool hasOutput, const std::string& outputType) {
    return hasOutput ? outputType : std::string("void");
}

std::string StackNode::memberType(bool hasOutput, const std::string& outputMemberType) {
    return hasOutput ? outputMemberType : std::string("void");
}

bool StackNode::keepsResult(const std::string& buildStage, const std::string& outputType) {
    // Discarded only when both hold: the third stage and an empty type.
    if (buildStage != "generate") return true;
    if (outputType != "void") return true;
    return false;
}

bool StackNode::fallsBackToItself(bool hasOutput) {
    return !hasOutput;
}

bool StackNode::restoresOnThrow() {
    return false;
}

bool StackNode::isCacheable() {
    return false;
}
