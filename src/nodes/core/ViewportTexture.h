#pragma once
#include <string>

// Port of the defaults in src/nodes/display/ViewportTextureNode.js at the pinned revision. Two
// of them disagree with what a reader would expect, and both disagreements are recorded here
// rather than tidied away.
class ViewportTexture {
  public:
    static constexpr const char* kTypeName = "ViewportTextureNode";

    // The flag the node sets for type testing. It does not name this class: it says the node is
    // an output texture, which is the name a test has to use. A reader looking for a flag
    // matching the class name would not find one.
    static constexpr const char* kFlagName = "isOutputTextureNode";

    // The filter a framebuffer texture made for the node is given. It is a mipmapped filter.
    static constexpr const char* kMinFilter = "LinearMipmapLinearFilter";

    // Whether the node asks for mipmaps to be generated. It does not, although the filter it
    // just set is one that reads them; the two settings therefore point in opposite directions
    // and only a caller that turns mipmap generation on will see the filter do anything.
    static constexpr bool generatesMipmaps = false;

    // Whether the filter and the mipmap setting agree, which they do not.
    static bool filterAndMipmapsAgree();

    // The kind of refresh the node asks for. It is a refresh before the render, not after it,
    // since the contents of the bound framebuffer are read at the start of a render.
    static constexpr const char* kUpdateBeforeType = "RENDER";

    // Whether a framebuffer texture is made when none was given. The node makes one, and keeps a
    // record of which texture belongs to which render target so that two targets do not share
    // one.
    static bool makesDefaultTexture(bool hasGivenTexture);

    // Whether the node keeps one texture per render target.
    static constexpr bool cachesPerRenderTarget = true;
};
