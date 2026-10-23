#pragma once
#include <cstdint>
#include <string>

// Port of the rules shared by the pass nodes in src/nodes/display/PassNode.js at the pinned
// revision. The module is large, so what is taken here is what the two texture classes settle:
// whether they may be cached, how they clone, and how many textures a pass may hold.
class PassTexture {
  public:
    static constexpr const char* kTypeName = "PassTextureNode";
    static constexpr const char* kMultipleTypeName = "PassMultipleTextureNode";

    // Whether the node may be cached. It may, which is the first time in this layer that the
    // answer is yes: every other node whose caching rule has been taken refuses outright. A read
    // that assumed the refusal was a rule of the layer rather than a choice per module would be
    // wrong here.
    static bool isCacheable();

    // Whether the node asks for its matrix to be updated. It does not, since it reads a texture
    // that the pass has already written rather than a texture given by a caller.
    static constexpr bool updatesMatrix = false;

    // How many textures a texture node holds. One for the plain node; the multiple kind exists
    // so that a caller can reach the one before as well as the current one.
    static int textureCount(bool isMultiple);

    // What a clone carries. The source builds a new node from the same pass and the same value,
    // so the clone is a second view of one texture rather than a copy of it.
    static bool cloneSharesTexture();

    // The number of textures the multiple kind keeps, which is two: the current one and the one
    // before it, which the previous texture API reaches.
    static constexpr int kMultipleTextureCount = 2;
};
