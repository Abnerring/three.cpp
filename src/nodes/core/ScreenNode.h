#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/display/ScreenNode.js at the pinned revision. The node reports
// one of four screen measurements; what is portable is how its type follows the measurement, how
// often it is refreshed, and how the viewport is worked out with or without a render target.
class ScreenNode {
  public:
    static constexpr const char* kTypeName = "ScreenNode";
    static constexpr bool isViewportNode = true;

    // The four measurements, named as the class names them.
    static const std::vector<std::string>& scopes();

    // The type each measurement reports. Only the viewport is four parts wide; the coordinate,
    // the size and the normalised pair are all two.
    static std::string nodeType(const std::string& scope);

    // How often the node is refreshed. The size and the viewport are read from the renderer and
    // refreshed every render; the other two are worked out in the shader and need no refresh at
    // all, which the source spells by giving them the none kind rather than leaving it unset.
    static std::string updateType(const std::string& scope);

    // Whether the pixel ratio is applied to a viewport. It is applied only when there is no
    // render target, since a target's own viewport is already counted in physical pixels and
    // multiplying again would double count.
    static bool appliesPixelRatio(bool hasRenderTarget);

    // Whether the node may be cached. It may not, as with the other modules that refuse.
    static bool isCacheable();

    // The vertical flip applied to the fragment coordinate when the builder says the image is
    // turned over, which the source records as following the newer convention.
    static double flippedY(double sizeY, double coordinateY);

    // Whether a viewport is read from the target rather than from the renderer.
    static bool viewportFromTarget(bool hasRenderTarget);
};
