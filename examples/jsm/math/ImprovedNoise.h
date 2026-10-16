#pragma once

// Native port of examples/jsm/math/ImprovedNoise.js from the pinned three.js revision
// (see upstream.json).
//
// The JavaScript module keeps its 512 entry permutation table, `fade()` and `grad()` in
// module scope and exports one class with a single `noise( x, y, z )` method. The table is
// built once at import time and never mutated afterwards, so the native port keeps it in the
// translation unit; `ImprovedNoise` itself carries no state, exactly like the source class.
//
// `noise()` returns a plain binary64 number. Every factor, sum and `lerp()` nesting level is
// written in the source's own evaluation order, including the `Math.floor()` / `& 255`
// lattice reduction and the sign rules of `grad()`.

class ImprovedNoise {
  public:
    /**
     * Returns a noise value for the given parameters.
     *
     * @param x The x coordinate.
     * @param y The y coordinate.
     * @param z The z coordinate.
     * @return The noise value.
     */
    double noise(double x, double y, double z) const;
};
