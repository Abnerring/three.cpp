#pragma once
#include <string>

// Port of the rules in src/nodes/utils/TriplanarTextures.js at the pinned revision. The mapping
// takes three samples and blends them by how much the surface faces each axis; two of its
// details do not follow from the name.
class TriplanarTextures {
  public:
    static constexpr const char* kTypeName = "TriplanarTextures";

    // Three weights, one per axis.
    struct Weights {
        double x = 0, y = 0, z = 0;
    };

    // The blend weights for a normal. The parts are taken absolute, normalised, and then divided
    // by their own sum, which makes the three add to one whatever the normal was; normalising
    // alone would not, since the sum of three normalised parts is not one.
    static Weights weights(double nx, double ny, double nz);

    // The sum the weights are divided by, which is the sum of the absolute parts after
    // normalising. It is the same for a normal and its negation.
    static double weightSum(double nx, double ny, double nz);

    // The two coordinates a sample takes for each axis. The pairs are y z, z x and x y: the
    // middle one is the third then the first rather than the first then the third, so the three
    // run in a cycle instead of each being the pair the axis does not have. A mapping built from
    // the axis's complement would give x z for the middle and would mirror that texture.
    struct Pair {
        double u = 0, v = 0;
    };

    static Pair sampleCoordinates(char axis, double px, double py, double pz, double scale);

    // Which texture is used when one was not given. The first stands in for the other two, so a
    // single texture gives all three samples.
    static constexpr bool missingTexturesFallBackToFirst = true;
};
