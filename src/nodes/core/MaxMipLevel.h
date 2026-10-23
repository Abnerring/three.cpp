#pragma once
#include <cstdint>
#include <string>

// Port of the rules in src/nodes/utils/MaxMipLevelNode.js at the pinned revision. The node
// answers with the deepest mipmap level a texture can have, which is worked out from one of its
// two dimensions rather than from both.
class MaxMipLevel {
  public:
    static constexpr const char* kTypeName = "MaxMipLevelNode";

    // The deepest level for a size. It comes from the larger of the two dimensions, because a
    // mipmap chain stops when the smaller dimension has been halved down to one; a wide thin
    // texture therefore has as many levels as its width gives, not as few as its height. A
    // sixteen by four texture has four levels rather than two.
    static double level(double width, double height);

    // Whether a size is usable at all. The test is against the absence of a width rather than
    // for its truth, so a width of nought passes it; the level then comes out as the logarithm
    // of the other dimension or, when both are nought, as minus infinity.
    static bool usable(bool hasWidth);

    // What level a width of nought gives when the height is positive, which is the larger
    // dimension's logarithm.
    static double levelWithZeroWidth(double height);

    // The kind of refresh the node asks for. It is a frame one, unlike the screen and viewport
    // nodes, which ask for a render one.
    static constexpr const char* kUpdateType = "FRAME";

    // The starting value, before any update has run.
    static constexpr double kInitialValue = 0.0;

    // How an image is found among the three places a texture may keep one, in the order the
    // source tries them.
    enum class ImageSource { FirstImage, FirstImageOrItself, TextureImage };

    static ImageSource imageSource(bool hasImages, bool hasFirstImage, bool firstImageHasImage);
};
