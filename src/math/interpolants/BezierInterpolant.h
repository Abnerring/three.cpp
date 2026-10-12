#pragma once
#include "math/Interpolant.h"

class BezierInterpolant : public Interpolant {
  public:
    using Interpolant::Interpolant;
    Array inTangents, outTangents;
    Array interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) override;
};
