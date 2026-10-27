#pragma once
#include <string>

// Port of the rules in src/nodes/display/NormalMapNode.js at the pinned revision. The node
// applies a normal map; what is portable is how a sampled colour becomes a direction and which
// two of its four parts a packed map is read from.
class NormalMap {
  public:
    static constexpr const char* kTypeName = "NormalMapNode";

    // The three ways a normal map may be packed, named as the constants module names them.
    enum class Packing { None, RedGreen, GreenAlpha };

    // Two of them are named after the channels they use, which is the part worth writing down:
    // a red green map keeps its two components in the first two parts, while a green alpha map
    // keeps them in the second and the fourth. Reading the second as if it were the first two
    // would give the wrong direction for every pixel of such a map.
    enum class Channels { Whole, Xy, Yw };

    static Channels channelsFor(Packing packing);

    // Whether a packing mode is one the node does not handle, which it reports as an error but
    // does not stop for: the colour is left as it was and the rest of the setup carries on.
    static bool isUnexpected(Packing packing);

    // The message printed for an unexpected mode, with the mode's name in it.
    static std::string unexpectedModeMessage(const std::string& modeName);

    // How a sampled colour becomes a direction: twice the value less one, taking it from the
    // range nought to one into the range minus one to one.
    static double signedValue(double sampled);
};

#include "core/JSValue.h"
JSValue nativeNormalMapNodeConstructor();
JSValue nativeNormalMapNodeModule();
JSValue nativeNormalMapNodeBinding(const std::string& name);
