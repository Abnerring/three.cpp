#include "nodes/core/SplitNode.h"

#include <algorithm>

namespace {
const char* kComponents = "xyzw";
}

int SplitNode::vectorLength(const std::string& components) {
    int length = static_cast<int>(components.size());
    for (const char c : components) {
        const char* found = std::find(kComponents, kComponents + 4, c);
        const int index = (found == kComponents + 4) ? -1 : static_cast<int>(found - kComponents);
        length = std::max(index + 1, length);
    }
    return length;
}

bool SplitNode::swizzleIsIdentity(int nodeTypeLength, const std::string& components) {
    // As many names as the value is wide, and the first that many in order.
    if (static_cast<int>(components.size()) != nodeTypeLength) return false;
    return components == std::string(kComponents, static_cast<std::size_t>(nodeTypeLength));
}

bool SplitNode::componentsAreRead(int nodeTypeLength) {
    // A one part value ignores them entirely.
    return nodeTypeLength > 1;
}

bool SplitNode::isCacheable() {
    return false;
}
