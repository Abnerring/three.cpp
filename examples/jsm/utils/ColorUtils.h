#pragma once

#include "math/Color.h"

// Native port of examples/jsm/utils/ColorUtils.js from the pinned three.js revision
// (see upstream.json).
//
// The addon has one helper: setKelvin writes the black body colour of a temperature in kelvin
// onto a colour, using the well known piecewise approximation that the pinned file transcribes.
// The clamp and the arithmetic follow the pinned order exactly so the result is bit comparable
// up to the standard library transcendental functions.
namespace ColorUtils {
Color& setKelvin(Color& color, double kelvin);
}
