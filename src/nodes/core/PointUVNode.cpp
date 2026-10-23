#include "nodes/core/PointUVNode.h"

bool PointUVNode::isCacheable() {
    // The answer is a plain no rather than a question asked of anything.
    return false;
}

std::string PointUVNode::generate() {
    // The vertical coordinate is turned over with one minus itself, while the horizontal one is
    // passed through as it is.
    return "vec2( gl_PointCoord.x, 1.0 - gl_PointCoord.y )";
}
