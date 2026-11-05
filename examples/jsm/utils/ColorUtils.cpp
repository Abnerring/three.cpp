#include "utils/ColorUtils.h"

#include "constants.h"

#include <cmath>

namespace {
double clamp(double value, double minimum, double maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}
}  // namespace

Color& ColorUtils::setKelvin(Color& color, double kelvin) {
    const double temp = clamp(kelvin, 1000, 40000) / 100;
    double r;
    double g;
    double b;

    if (temp <= 66) {
        r = 255;
    } else {
        r = 329.698727446 * std::pow(temp - 60, -0.1332047592);
    }

    if (temp <= 66) {
        g = 99.4708025861 * std::log(temp) - 161.1195681661;
    } else {
        g = 288.1221695283 * std::pow(temp - 60, -0.0755148492);
    }

    if (temp >= 66) {
        b = 255;
    } else if (temp <= 19) {
        b = 0;
    } else {
        b = 138.5177312231 * std::log(temp - 10) - 305.0447927307;
    }

    return color.setRGB(clamp(r, 0, 255) / 255, clamp(g, 0, 255) / 255, clamp(b, 0, 255) / 255,
                        SRGBColorSpace);
}
