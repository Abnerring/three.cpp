#include "nodes/core/BypassNode.h"

std::string BypassNode::nodeType(const std::string& outputType) {
    // The output's type, not the call's.
    return outputType;
}

bool BypassNode::addsLine(const std::string& snippet) {
    // An empty text adds nothing, so no blank line is produced.
    return !snippet.empty();
}

bool BypassNode::buildsCallFirst() {
    return true;
}

bool BypassNode::isCacheable() {
    return false;
}
