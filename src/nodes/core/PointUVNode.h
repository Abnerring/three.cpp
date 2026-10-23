#pragma once
#include <string>

// Port of src/nodes/core/PointUVNode.js at the pinned revision. The node answers with the
// coordinates a point primitive's fragment sits at, which a shader may use to draw a sprite over
// it. Two things about it are worth writing down: the vertical coordinate is turned over before
// it is used, so that the shader's idea of up matches the image's; and the node says it may
// never be cached, which is the opposite of what a node that answers with the same thing every
// time would say.
class PointUVNode {
  public:
    static constexpr bool isPointUVNode = true;
    static constexpr const char* kTypeName = "PointUVNode";

    // The type the node answers with.
    static constexpr const char* kNodeType = "vec2";

    // Whether the node may be cached. It may not: the node says so plainly rather than by
    // working anything out, so a caller must not reuse its answer.
    static bool isCacheable();

    // The shader text the node generates. The vertical coordinate is turned over, since the
    // point coordinate counts downwards while the image counts upwards.
    static std::string generate();
};
