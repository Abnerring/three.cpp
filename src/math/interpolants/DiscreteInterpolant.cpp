#include "math/interpolants/DiscreteInterpolant.h"
Interpolant::Array DiscreteInterpolant::interpolate_(std::ptrdiff_t i1, JSValue, double, JSValue) {
    return copySampleValue_(i1 - 1);
}
