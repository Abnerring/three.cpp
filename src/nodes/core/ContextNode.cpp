#include "nodes/core/ContextNode.h"

std::string ContextNode::scope(const std::string& wrappedScope) {
    return wrappedScope;
}

std::string ContextNode::nodeType(const std::string& wrappedType) {
    return wrappedType;
}

std::string ContextNode::memberType(const std::string& wrappedMemberType) {
    return wrappedMemberType;
}

std::map<std::string, int> ContextNode::gather(
    const std::vector<std::map<std::string, int>>& gatheredInOrder) {
    // A fresh object into which each is merged in turn, so the later wins.
    std::map<std::string, int> out;
    for (const auto& entry : gatheredInOrder) {
        for (const auto& pair : entry) out[pair.first] = pair.second;
    }
    return out;
}

bool ContextNode::appliesContext(int usageCountAfterIncrement) {
    // True only on the first use.
    return usageCountAfterIncrement == 1;
}

bool ContextNode::restoresOnThrow() {
    // A plain restore after the call.
    return false;
}

bool ContextNode::isCacheable() {
    return false;
}
