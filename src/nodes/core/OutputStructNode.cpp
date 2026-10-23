#include "nodes/core/OutputStructNode.h"

std::string OutputStructNode::memberName(int index) {
    // Positional names: m nought, m one, and so on.
    return "m" + std::to_string(index);
}

std::string OutputStructNode::structPrefix(const std::string& propertyName) {
    // A name that is there takes a dot after it; an empty one gives nothing rather than a dot.
    return propertyName.empty() ? std::string() : propertyName + ".";
}

std::string OutputStructNode::memberLine(const std::string& propertyName, int index,
                                         const std::string& snippet) {
    return structPrefix(propertyName) + memberName(index) + " = " + snippet;
}

std::string OutputStructNode::generateAnswer(const std::string& propertyName) {
    return propertyName;
}

bool OutputStructNode::layoutIsCached() {
    return true;
}

bool OutputStructNode::isCacheable() {
    return false;
}
