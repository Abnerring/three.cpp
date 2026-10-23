#include "nodes/core/ModelNode.h"

const std::vector<std::string>& ModelNode::scopes() {
    // The five names the constructor documents, in the order it lists them.
    static const std::vector<std::string> names{"position", "viewPosition", "direction", "scale",
                                               "worldMatrix"};
    return names;
}

bool ModelNode::isScope(const std::string& name) {
    for (const auto& scope : scopes()) {
        if (scope == name) return true;
    }
    return false;
}
