#include "nodes/core/BuiltinNode.h"

bool BuiltinNode::isCacheable() {
    // A plain no, as for the point coordinate node.
    return false;
}

std::string BuiltinNode::generate() const {
    // The name is handed back untouched, with no check that the shader knows it.
    return name_;
}
