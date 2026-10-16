#pragma once

// Native port of examples/jsm/math/ColorConverter.js from the pinned three.js revision
// (see upstream.json).
//
// Upstream holds one module-scope scratch record `_hsl = {}` that every `getHSV()` call
// reuses, and writes its HSV result into the caller's plain `{ h, s, v }` target object.
// The native port models both records the way `Color::getHSL()` / `Color::setHSL()` model
// their `{ h, s, l }` records, with a `Vector3`: `x` is `h`, `y` is `s` and `z` is `v`.
// The scratch record itself stays private to the translation unit, exactly like the
// module-scope object.
//
// `setHSV()` returns whatever `color.setHSL()` returns upstream, which is the color itself,
// so it returns a reference to the same `Color` here.

class Color;
class Vector3;

class ColorConverter {
  public:
    /**
     * Sets the given HSV color definition to the given color object.
     *
     * @param color The color to set.
     * @param h The hue.
     * @param s The saturation.
     * @param v The value.
     * @return The updated color.
     */
    static Color& setHSV(Color& color, double h, double s, double v);

    /**
     * Returns a HSV color representation of the given color object.
     *
     * @param color The color to get HSV values from.
     * @param target The target vector that is used to store the result (`x` = h, `y` = s,
     *               `z` = v).
     * @return The HSV color.
     */
    static Vector3& getHSV(const Color& color, Vector3& target);
};
