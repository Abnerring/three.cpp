#include "math/interpolants/BezierInterpolant.h"
#include <algorithm>
#include <cmath>

namespace {
double cubicBezier(double s, const JSValue& p0, const JSValue& p1,
                   const JSValue& p2, const JSValue& p3) {
    const auto k = 1 - s;
    auto result = k * k * k * p0.toNumber();
    result += 3 * k * k * s * p1.toNumber();
    result += 3 * k * s * s * p2.toNumber();
    return result + s * s * s * p3.toNumber();
}
double cubicBezierSlope(double s, const JSValue& p0, const JSValue& p1,
                        const JSValue& p2, const JSValue& p3) {
    const auto k = 1 - s;
    auto result = 3 * k * k * Interpolant_detail::subtract(p1, p0);
    result += 6 * k * s * Interpolant_detail::subtract(p2, p1);
    return result + 3 * s * s * Interpolant_detail::subtract(p3, p2);
}
double solveBezierParameter(double x, const JSValue& x0, const JSValue& x1,
                             const JSValue& x2, const JSValue& x3) {
    const auto numerator = Interpolant_detail::subtract(x, x0);
    const auto denominator = Interpolant_detail::subtract(x3, x0);
    auto s = numerator / denominator;
    for (int i = 0; i < 8; ++i) {
        const auto error = cubicBezier(s, x0, x1, x2, x3) - x;
        if (std::abs(error) < 1e-10)
            break;
        const auto slope = cubicBezierSlope(s, x0, x1, x2, x3);
        if (std::abs(slope) < 1e-10)
            break;
        const auto next = s - error / slope;
        s = std::isnan(next) ? next : std::max(0.0, std::min(1.0, next));
    }
    return s;
}
}
Interpolant::Array BezierInterpolant::interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) {
    auto result = resultBuffer;
    const auto values = sampleValues;
    const auto stride = valueSize;
    requireFiniteStride(stride);
    const auto offset1 = double(i1) * double(stride), offset0 = offset1 - double(stride);
    const auto in = inTangents, out = outTangents;
    if (!in.toBoolean() || !out.toBoolean()) {
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
    const auto tangentStride = double(stride) * 2;
    const auto i0 = i1 - 1;
    for (double i = 0; i != stride; ++i) {
        const auto v0 = values.get(offset0 + double(i)), v1 = values.get(offset1 + double(i));
        const auto outOffset = double(i0) * tangentStride + double(i) * 2;
        const auto c0x = out.get(outOffset), c0y = out.get(outOffset + 1);
        const auto inOffset = double(i1) * tangentStride + double(i) * 2;
        const auto c1x = in.get(inOffset), c1y = in.get(inOffset + 1);
        const auto s = solveBezierParameter(t, t0, c0x, c1x, t1);
        result.set(double(i), cubicBezier(s, v0, c0y, c1y, v1));
    }
    return result;
}
