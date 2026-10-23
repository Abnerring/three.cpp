#include "nodes/core/ViewportSharedTextureNode.h"

bool ViewportSharedTextureNode::createsTheSharedTextureOnce() {
    return true;
}

bool ViewportSharedTextureNode::textureIgnoresTheInstance() {
    return true;
}

bool ViewportSharedTextureNode::updateReferenceIsANoOp() {
    return true;
}
