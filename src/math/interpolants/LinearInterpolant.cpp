#include "math/interpolants/LinearInterpolant.h"
Interpolant::Array LinearInterpolant::interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) {
    auto result = resultBuffer;
    const auto values = sampleValues;
    const auto stride = valueSize;
    requireFiniteStride(stride);
    const auto offset1 = double(i1) * double(stride), offset0 = offset1 - double(stride);
    const auto numerator = Interpolant_detail::subtract(t, t0);
    const auto denominator = Interpolant_detail::subtract(t1, t0);
    const auto weight1 = numerator / denominator, weight0 = 1 - weight1;
    for (double i = 0; i != stride; ++i) {
        const auto first = values.number(offset0 + double(i)) * weight0;
        const auto second = values.number(offset1 + double(i)) * weight1;
        result.set(double(i), first + second);
    }
    return result;
}
