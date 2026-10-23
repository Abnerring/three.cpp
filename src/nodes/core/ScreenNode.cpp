#include "nodes/core/ScreenNode.h"

const std::vector<std::string>& ScreenNode::scopes() {
    static const std::vector<std::string> names{"coordinate", "viewport", "size", "uv"};
    return names;
}

std::string ScreenNode::nodeType(const std::string& scope) {
    // Only the viewport is a four part rectangle.
    if (scope == "viewport") return "vec4";
    return "vec2";
}

std::string ScreenNode::updateType(const std::string& scope) {
    // The size and the viewport come from the renderer and are refreshed every render; the
    // other two need no refresh.
    if (scope == "size" || scope == "viewport") return "RENDER";
    return "NONE";
}

bool ScreenNode::appliesPixelRatio(bool hasRenderTarget) {
    // Only the default framebuffer's viewport needs the ratio; a target's is already physical.
    return !hasRenderTarget;
}

bool ScreenNode::isCacheable() {
    return false;
}

double ScreenNode::flippedY(double sizeY, double coordinateY) {
    // The size less the coordinate, which turns the image over about its middle.
    return sizeY - coordinateY;
}

bool ScreenNode::viewportFromTarget(bool hasRenderTarget) {
    return hasRenderTarget;
}
