#pragma once
#include <string>

// Port of the rules in src/nodes/utils/SampleNode.js at the pinned revision. The node hands a
// coordinate to a callback the caller supplied, and one of its two arguments turns out to have
// no effect at all.
class SampleNode {
  public:
    static constexpr const char* kTypeName = "SampleNode";

    // The coordinate the setup passes to the callback. The constructor accepts a coordinate and
    // stores it, but the setup calls the sampling with a freshly made default coordinate
    // instead, so the one that was given is never read. A caller that passes its own coordinate
    // therefore gets the default one, and the argument is dead.
    enum class CoordinateSource { Given, Default };

    static CoordinateSource sourceUsedInSetup();

    // Whether the coordinate the constructor was given is ever read, which it is not.
    static constexpr bool givenCoordinateIsRead = false;

    // Whether the node may be cached. It may not, as with the other nodes in this layer that
    // refuse outright.
    static bool isCacheable();

    // What the sampling answers: the callback's answer for the coordinate it was handed. The
    // callback is the only thing that decides it, so the node adds nothing of its own.
    static double sample(double callbackAnswer);
};
