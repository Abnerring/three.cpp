#pragma once
#include <cstdint>
#include <memory>

// Port of the sharing rule in src/nodes/display/ViewportDepthTextureNode.js at the pinned
// revision. The node reads the viewport's depth as a texture; when it is not given one it uses a
// texture shared by every such node, which is made the first time it is needed and kept
// afterwards.
class ViewportDepthTexture {
  public:
    static constexpr const char* kTypeName = "ViewportDepthTextureNode";

    // How many textures may be made for the default case. It is one, however many nodes ask,
    // which is the whole point of the sharing.
    static std::uint32_t defaultTextureCount();

    // The number of nodes that have asked for a default texture, so that a second node can be
    // seen to reuse the first one's rather than making another.
    static std::uint32_t nodesServed();

    // A node asking for a texture. A texture given to it is used as it stands, without being
    // checked; one that is exactly absent puts the shared texture in its place, making it if
    // this is the first time. The number reported is the same for every default caller, which is
    // what sharing means, and differs from a texture given in.
    static std::uint64_t textureId(bool hasOwnTexture, std::uint64_t ownTextureId);

    // Forgets the shared texture and the count, so that a test starts from a clean position.
    static void reset();
};
