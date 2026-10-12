#include "math/interpolants/CubicInterpolant.h"
#include "constants.h"

CubicInterpolant::CubicInterpolant(Array positions, Array values, double size, Array result)
    : Interpolant(std::move(positions), std::move(values), size, std::move(result)) {
    DefaultSettings_.set("endingStart", ZeroCurvatureEnding);
    DefaultSettings_.set("endingEnd", ZeroCurvatureEnding);
}
void CubicInterpolant::intervalChanged_(std::ptrdiff_t i1, JSValue t0, JSValue t1) {
    const auto pp = parameterPositions;
    auto iPrev = i1 - 2, iNext = i1 + 1;
    auto tPrev = pp.get(double(iPrev)), tNext = pp.get(double(iNext));
    if (tPrev.isUndefined()) {
        const auto ending = dispatchGetSettings_().get("endingStart");
        if (ending.strictEquals(ZeroSlopeEnding)) {
            iPrev = i1;
            const auto doubled = 2 * t0.toNumber();
            tPrev = Interpolant_detail::subtract(doubled, t1);
        } else if (ending.strictEquals(WrapAroundEnding)) {
            iPrev = static_cast<std::ptrdiff_t>(pp.size()) - 2;
            const auto sum = Interpolant_detail::add(t0, pp.get(double(iPrev)));
            tPrev = Interpolant_detail::subtract(sum, pp.get(double(iPrev) + 1));
        } else {
            iPrev = i1;
            tPrev = t1;
        }
    }
    if (tNext.isUndefined()) {
        const auto ending = dispatchGetSettings_().get("endingEnd");
        if (ending.strictEquals(ZeroSlopeEnding)) {
            iNext = i1;
            const auto doubled = 2 * t1.toNumber();
            tNext = Interpolant_detail::subtract(doubled, t0);
        } else if (ending.strictEquals(WrapAroundEnding)) {
            iNext = 1;
            const auto sum = Interpolant_detail::add(t1, pp.get(1));
            tNext = Interpolant_detail::subtract(sum, pp.get(0));
        } else {
            iNext = i1 - 1;
            tNext = t0;
        }
    }
    const auto halfDt = Interpolant_detail::subtract(t1, t0) * 0.5;
    const auto stride = valueSize;
    _weightPrev = halfDt / Interpolant_detail::subtract(t0, tPrev);
    _weightNext = halfDt / Interpolant_detail::subtract(tNext, t1);
    _offsetPrev = double(iPrev) * stride;
    _offsetNext = double(iNext) * stride;
}
Interpolant::Array CubicInterpolant::interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) {
    auto result = resultBuffer;
    const auto values = sampleValues;
    const auto stride = valueSize;
    requireFiniteStride(stride);
    const auto o1 = double(i1) * double(stride), o0 = o1 - double(stride);
    const auto oP = _offsetPrev, oN = _offsetNext, wP = _weightPrev, wN = _weightNext;
    const auto numerator = Interpolant_detail::subtract(t, t0);
    const auto denominator = Interpolant_detail::subtract(t1, t0);
    const auto p = numerator / denominator, pp = p * p, ppp = pp * p;
    const auto sP = -wP * ppp + 2 * wP * pp - wP * p;
    const auto s0 = (1 + wP) * ppp + (-1.5 - 2 * wP) * pp + (-0.5 + wP) * p + 1;
    const auto s1 = (-1 - wN) * ppp + (1.5 + wN) * pp + 0.5 * p;
    const auto sN = wN * ppp - wN * pp;
    for (double i = 0; i != stride; ++i) {
        const auto previous = sP * values.number(oP + double(i));
        const auto first = s0 * values.number(o0 + double(i));
        const auto second = s1 * values.number(o1 + double(i));
        const auto next = sN * values.number(oN + double(i));
        result.set(double(i), previous + first + second + next);
    }
    return result;
}
