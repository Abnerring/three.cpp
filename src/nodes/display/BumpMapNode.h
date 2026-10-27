#pragma once
#include <string>

// Port of the rules in src/nodes/display/BumpMapNode.js at the pinned revision. The node works
// out a normal from a height map using screen space derivatives, which two decisions in it guard
// and choose. Both use strict comparisons, and both differ from what a looser test would do.
class BumpMap {
  public:
    static constexpr const char* kTypeName = "BumpMapNode";

    // Whether the bump is skipped entirely. Screen space derivatives are unreliable on the thin
    // lines a wireframe draws, so a material drawn that way gets the plain normal instead. The
    // test is strict, so a material that never said whether it is wireframe is not skipped.
    static bool skipsBump(bool wireframeIsTrue);

    // The scale the height differences are multiplied by. A scale that is there is used even
    // when it is nought, since the test is against null rather than for truth: a material that
    // deliberately flattens its bumps should stay flat rather than jumping to one.
    static double bumpScale(bool hasScale, double scale);

    // The determinant the gradient is divided by, before the facing is taken into account.
    static double determinant(double sigmaXDotR1);

    // The factor the gradient is multiplied by, which is the sign of the determinant rather than
    // its size; taking the size would lose which way the surface turns.
    static double gradientSign(double determinant);
};

#include "core/JSValue.h"
JSValue nativeBumpMapNodeConstructor();
JSValue nativeBumpMapNodeModule();
JSValue nativeBumpMapNodeBinding(const std::string& name);
