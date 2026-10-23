#pragma once
#include <string>

// Port of the rules in src/nodes/display/ViewportDepthTextureNode.js at the pinned revision. The
// node reads the viewport's depth as a texture, and its one rule is about which texture it reads.
class ViewportDepthTextureNode {
  public:
    static constexpr const char* kTypeName = "ViewportDepthTextureNode";

    // The texture a node reads. A texture given when the node is made is used as it stands; only
    // an absent one is replaced by the shared depth texture, and the test is for the absence
    // rather than for a falsy value, so any texture passed in is respected.
    enum class Source { Given, Shared };

    static Source sourceFor(bool textureWasGiven);

    // Whether the shared depth texture is made once and reused, which it is: the first node to
    // exist makes it and every later one reads the same object.
    static bool sharesOneDepthTexture();

    // Whether the node has the reference method overridden the way its shared sibling has. It
    // does not: this node keeps the ordinary behaviour, so only the texture it starts from
    // differs.
    static bool overridesReferenceUpdate();

    // The parameters a node takes, which is the coordinate, a level, and a texture.
    static constexpr int kParameterCount = 3;
};
