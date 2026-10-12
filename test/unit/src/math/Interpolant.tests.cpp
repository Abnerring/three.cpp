#include "catch.hpp"
#include "math/Interpolant.h"
#include "math/interpolants/LinearInterpolant.h"
#include <limits>

namespace {
class RecordingInterpolant : public Interpolant {
  public:
    using Interpolant::Interpolant;
    std::vector<std::ptrdiff_t> changed;
    void intervalChanged_(std::ptrdiff_t index, JSValue, JSValue) override { changed.push_back(index); }
    Array interpolate_(std::ptrdiff_t index, JSValue, double, JSValue) override {
        return copySampleValue_(index - 1);
    }
};
class CustomSplineInterpolant : public Interpolant {
  public:
    using Interpolant::Interpolant;
    Array copySampleValue_(std::ptrdiff_t index) override {
        auto result = resultBuffer;
        for (double i = 0; i != valueSize; ++i)
            result.set(i, sampleValues.get(double(index) * valueSize * 3 + valueSize + i));
        return result;
    }
    Array interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) override {
        auto result = resultBuffer;
        const auto td = Interpolant_detail::subtract(t1, t0);
        const auto p = Interpolant_detail::subtract(t, t0) / td, pp = p * p, ppp = pp * p;
        const auto offset1 = double(i1) * valueSize * 3, offset0 = offset1 - valueSize * 3;
        const auto s2 = -2 * ppp + 3 * pp, s3 = ppp - pp, s0 = 1 - s2, s1 = s3 - pp + p;
        for (double i = 0; i != valueSize; ++i) {
            const auto p0 = sampleValues.number(offset0 + i + valueSize);
            const auto m0 = sampleValues.number(offset0 + i + valueSize * 2) * td;
            const auto p1 = sampleValues.number(offset1 + i + valueSize);
            const auto m1 = sampleValues.number(offset1 + i) * td;
            result.set(i, s0 * p0 + s1 * m0 + s2 * p1 + s3 * m1);
        }
        return result;
    }
};
}
TEST_CASE("Interpolant seek caches intervals and calls overrides only on transitions", "[interpolant]") {
    RecordingInterpolant value(Interpolant::Array::array({0, 1, 2, 3, 4, 5, 6}),
                               Interpolant::Array::array({10, 20, 30, 40, 50, 60, 70}), 1);
    for (const auto time : {0., .2, 1.2, 5.2, .3, -1., 7., 7., 4.2}) value.evaluate(time);
    REQUIRE(value.changed == std::vector<std::ptrdiff_t>{1, 2, 6, 1, 5});
    REQUIRE(value._cachedIndex == 5);
    REQUIRE(value.resultBuffer.number(0) == 50);
    REQUIRE(value.evaluate(std::numeric_limits<double>::quiet_NaN()).number(0) == 70);
    REQUIRE(value._cachedIndex == 7);
}
TEST_CASE("Interpolant ordinary arrays preserve undefined and result object identity", "[interpolant]") {
    const auto values = Interpolant::Array::array({JSValue{}, std::numeric_limits<double>::quiet_NaN(),
                                                  nullptr, "value", true, -0.0});
    const auto destination = JSValue::array();
    Interpolant value(nullptr, values, 2, destination);
    REQUIRE(value.copySampleValue_(0).sameIdentity(destination));
    REQUIRE(value.resultBuffer.get(0).isUndefined());
    REQUIRE(std::isnan(value.resultBuffer.number(1)));
    REQUIRE(value.copySampleValue_(1).get(0).isNull());
    REQUIRE(value.resultBuffer.get(1).string() == "value");
    REQUIRE(value.copySampleValue_(2).get(0).strictEquals(true));
    REQUIRE(std::signbit(value.resultBuffer.number(1)));
}
TEST_CASE("Interpolant abstract behavior remains extensible at boundaries", "[interpolant]") {
    Interpolant value(Interpolant::Array::array({0, 1}), Interpolant::Array::array({3, 9}), 1);
    REQUIRE(value.evaluate(-1).number(0) == 3);
    REQUIRE(value.evaluate(2).number(0) == 9);
    REQUIRE_THROWS_WITH(value.evaluate(.5), "THREE.Interpolant: Call to abstract method.");
    const auto defaults = value.getSettings_();
    REQUIRE(defaults.sameValue(value.DefaultSettings_));
    value.settings = JSValue::object();
    REQUIRE(value.getSettings_().sameValue(value.settings));
    value.settings = 0;
    REQUIRE(value.getSettings_().sameValue(defaults));
}
TEST_CASE("Interpolant typed sample and result views retain shared bytes", "[interpolant]") {
    Float64Array samples({1, 2, 3, 4, 5, 6});
    Float32Array positions({0, 1});
    auto result = samples.subarray(1, 4);
    LinearInterpolant value(positions, samples, 3, result);
    REQUIRE(value.parameterPositions.sameIdentity(positions));
    REQUIRE(value.sampleValues.sameIdentity(samples));
    REQUIRE(value.evaluate(.5).sameIdentity(result));
    REQUIRE(samples[1] == 2.5);
    REQUIRE(samples[2] == 3.75);
    REQUIRE(samples[3] == 4.875);
    positions[1] = 2;
    REQUIRE(value.evaluate(1).number(0) == 2.9375);
}
TEST_CASE("Interpolant constructors retain numeric strides and source storage constructors", "[interpolant]") {
    LinearInterpolant fractional(Float32Array({0, 1}), Float32Array({0, 1, 2, 3}), 1.5);
    REQUIRE(fractional.valueSize == 1.5);
    REQUIRE(fractional.resultBuffer.size() == 1);
    REQUIRE_THROWS_AS(fractional.evaluate(.5), JSRangeError);
    REQUIRE_THROWS_AS(LinearInterpolant(Interpolant::Array::array({0, 1}),
                                        Interpolant::Array::array({0, 1, 2, 3}), 1.5), JSRangeError);
    Interpolant withNull(nullptr, nullptr, 0, nullptr);
    REQUIRE(withNull.copySampleValue_(0).sameIdentity(nullptr));
}
TEST_CASE("Custom interpolants override boundary sample layout and interpolation", "[interpolant]") {
    CustomSplineInterpolant value(Interpolant::Array::array({0, 1}),
                                   Interpolant::Array::array({0, 0, 1, -1, 1, 0}), 1);
    REQUIRE(value.evaluate(0).number(0) == 0);
    REQUIRE(value.evaluate(1).number(0) == 1);
    REQUIRE(value.evaluate(.5).number(0) == .75);
    REQUIRE(value.evaluate(-1).number(0) == 0);
    REQUIRE(value.evaluate(2).number(0) == 1);
}
