#pragma once
#include "math/Interpolant.h"

class CubicInterpolant : public Interpolant {
  public:
    double _weightPrev = -0.0, _offsetPrev = -0.0;
    double _weightNext = -0.0, _offsetNext = -0.0;
    CubicInterpolant(Array parameterPositions, Array sampleValues, double sampleSize,
                     Array resultBuffer = {});
    void intervalChanged_(std::ptrdiff_t i1, JSValue t0, JSValue t1) override;
    Array interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) override;
};
