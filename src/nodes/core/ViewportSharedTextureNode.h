#pragma once
#include <cstdint>

// Port of the rules in src/nodes/display/ViewportSharedTextureNode.js at the pinned revision. The
// node is the per-instance viewport texture's shared sibling, and three of its rules are worth
// writing down.
class ViewportSharedTextureNode {
  public:
    static constexpr const char* kTypeName = "ViewportSharedTextureNode";

    // The one texture every instance uses. It is made the first time a node is constructed and
    // reused after that, so the first node to exist decides what the others read; there is no
    // per-instance texture at all, which is the whole difference from the ordinary viewport
    // texture node.
    static bool createsTheSharedTextureOnce();

    // Which texture a node answers with. It is the shared one whatever the node is, so the method
    // does not consult the node it is called on; a per-instance answer would defeat the point.
    static bool textureIgnoresTheInstance();

    // What updating the reference answers. The ordinary node builds a new reference for itself,
    // while this one answers itself unchanged, so the shared texture is never replaced. A caller
    // that took the answer for a new node would get the same one back.
    static bool updateReferenceIsANoOp();

    // The kind of refresh the node asks for, which is before the render.
    static constexpr const char* kUpdateType = "BEFORE_RENDER";
};
