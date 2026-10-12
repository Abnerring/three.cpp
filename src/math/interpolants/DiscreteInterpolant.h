#pragma once
#include "math/Interpolant.h"

class DiscreteInterpolant : public Interpolant {
  public:
    using Interpolant::Interpolant;
    Array interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) override;
};
