#include "nodes/core/VarNode.h"

bool VarNode::hasNameByDefault() {
    // The name starts absent, so the system generates one.
    return false;
}

bool VarNode::intentRequested(bool flag) {
    return flag;
}

bool VarNode::intentComesFromTheBuilder() {
    // The question is answered from the builder's record, not from the flag on the node.
    return true;
}

bool VarNode::setterReturnsTheNode() {
    return true;
}

bool VarNode::isCacheable() {
    return false;
}
