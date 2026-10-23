#include "nodes/core/Object3DNode.h"

const std::vector<std::string>& Object3DNode::scopes() {
    // The five the class documents, in the order it lists them.
    static const std::vector<std::string> names{"position", "viewPosition", "direction", "scale",
                                               "worldMatrix"};
    return names;
}

std::string Object3DNode::nodeType(const std::string& scope) {
    // Only the world matrix is a matrix; the rest are positions, a direction or a scale, all of
    // which are three part vectors. A scope the node does not know still answers with a vector,
    // since the source falls through to the same place rather than refusing.
    if (scope == "worldMatrix") return "mat4";
    return "vec3";
}

bool Object3DNode::isCacheable() {
    // A plain no, as for the point coordinate and builtin nodes.
    return false;
}
