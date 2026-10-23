#include "nodes/core/TempNode.h"

std::string TempNode::deprecationWarning() {
    return "TempNode: This module has been deprecated. Extend Node instead.";
}

bool TempNode::isCacheable(std::uint32_t usageCount) {
    // The rule is the same question as the one below, and the source answers the first by
    // asking the second rather than writing the comparison twice.
    return hasDependencies(usageCount);
}

bool TempNode::hasDependencies(std::uint32_t usageCount) {
    // More than one use. A node used once is not cached, and a node used none of the time is
    // certainly not.
    return usageCount > 1;
}
