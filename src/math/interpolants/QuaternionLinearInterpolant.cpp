#include "math/JSNumberMath.h"
#include "math/interpolants/QuaternionLinearInterpolant.h"
#include <array>
#include <cmath>

namespace {
void slerpFlat(InterpolantArray& result, const InterpolantArray& values,
               double offset0, double offset1, double t) {
    std::array<JSValue, 4> first{values.get(offset0), values.get(offset0 + 1),
                                values.get(offset0 + 2), values.get(offset0 + 3)};
    std::array<JSValue, 4> second{values.get(offset1), values.get(offset1 + 1),
                                 values.get(offset1 + 2), values.get(offset1 + 3)};
    if (!first[3].strictEquals(second[3]) || !first[0].strictEquals(second[0]) ||
        !first[1].strictEquals(second[1]) || !first[2].strictEquals(second[2])) {
        auto dot = Interpolant_detail::multiply(first[0], second[0]);
        dot += Interpolant_detail::multiply(first[1], second[1]);
        dot += Interpolant_detail::multiply(first[2], second[2]);
        dot += Interpolant_detail::multiply(first[3], second[3]);
        if (dot < 0) {
            for (auto& component : second)
                component = -component.toNumber();
            dot = -dot;
        }
        auto s = 1 - t;
        if (dot < 0.9995) {
            const auto theta = JSNumberMath::acos(dot), sine = JSNumberMath::sin(theta);
            s = JSNumberMath::sin(s * theta) / sine;
            t = JSNumberMath::sin(t * theta) / sine;
        }
        for (std::size_t i = 0; i < first.size(); ++i) {
            const auto previous = first[i].toNumber() * s;
            const auto next = second[i].toNumber() * t;
            first[i] = previous + next;
        }
        if (!(dot < 0.9995)) {
            const auto x0 = first[0].number(), y0 = first[1].number(),
                       z0 = first[2].number(), w0 = first[3].number();
            const auto f = 1 / std::sqrt(x0 * x0 + y0 * y0 + z0 * z0 + w0 * w0);
            for (auto& component : first)
                component = component.number() * f;
        }
    }
    for (std::size_t i = 0; i < first.size(); ++i)
        result.set(double(i), first[i]);
}
}
Interpolant::Array QuaternionLinearInterpolant::interpolate_(std::ptrdiff_t i1, JSValue t0,
                                                              double t, JSValue t1) {
    auto result = resultBuffer;
    const auto values = sampleValues;
    const auto stride = valueSize;
    requireFiniteStride(stride);
    if (std::fmod(stride, 4) != 0)
        throw JSRangeError("QuaternionLinearInterpolant sample size must be a multiple of four");
    const auto numerator = Interpolant_detail::subtract(t, t0);
    const auto denominator = Interpolant_detail::subtract(t1, t0);
    const auto alpha = numerator / denominator;
    auto offset = double(i1) * double(stride);
    for (const auto end = offset + double(stride); offset != end; offset += 4)
        slerpFlat(result, values, offset - double(stride), offset, alpha);
    return result;
}
