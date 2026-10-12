#include "catch.hpp"
#include "math/interpolants/DiscreteInterpolant.h"
#include "math/interpolants/LinearInterpolant.h"
#include "math/interpolants/CubicInterpolant.h"
#include "math/interpolants/QuaternionLinearInterpolant.h"
#include "math/interpolants/BezierInterpolant.h"
#include "constants.h"

namespace {
CubicInterpolant* changingCubic = nullptr;
JSValue numberWithCounter(const JSValue& counter, double number, bool incrementResult = false) {
    auto result = JSValue::object();
    result.set("valueOf", JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                              const std::vector<JSValue>& captures) {
        auto counter = captures[0];
        const auto count = counter.get("count").toNumber() + 1;
        counter.set("count", count);
        return captures[2].toBoolean() ? JSValue(count) : captures[1];
    }, {counter, number, incrementResult}));
    return result;
}
class OverriddenSettings : public CubicInterpolant {
  public:
    using CubicInterpolant::CubicInterpolant;
    JSValue overrideSettings = JSValue::object();
    mutable int calls = 0;
    JSValue getSettings_() const override { ++calls; return overrideSettings; }
};
}

TEST_CASE("Discrete and linear interpolants honor duplicate timestamps and typed conversion", "[interpolant]") {
    const auto positions = Interpolant::Array::array({0, 1, 1, 2});
    const auto samples = Interpolant::Array::array({1, 5, 9, 13});
    DiscreteInterpolant discrete(positions, samples, 1);
    REQUIRE(discrete.evaluate(.99).number(0) == 1);
    REQUIRE(discrete.evaluate(1).number(0) == 9);
    LinearInterpolant linear(positions, samples, 1);
    REQUIRE(linear.evaluate(1).number(0) == 9);
    REQUIRE(linear.evaluate(1.5).number(0) == 11);
    LinearInterpolant clamped(Interpolant::Array::array({0, 1}),
                              Interpolant::Array::array({0, 5}), 1, Uint8ClampedArray(1));
    REQUIRE(clamped.evaluate(.5).number(0) == 2);
}
TEST_CASE("Cubic settings are inherited dynamically but interval weights remain cached", "[interpolant]") {
    CubicInterpolant value(Interpolant::Array::array({0, 1, 3, 5}),
                           Interpolant::Array::array({0, 10, -5, 20}), 1);
    auto prototype = JSValue::object();
    prototype.set("endingStart", ZeroSlopeEnding);
    value.settings = JSValue::object(prototype);
    const auto initial = value.evaluate(.5).number(0);
    prototype.set("endingStart", WrapAroundEnding);
    REQUIRE(value.evaluate(.5).number(0) == initial);
    value.evaluate(2);
    REQUIRE(value.evaluate(.5).number(0) != initial);
    REQUIRE(value.getSettings_().sameValue(value.settings));
}
TEST_CASE("Quaternion interpolation follows original multi-quaternion result offset behavior", "[interpolant]") {
    QuaternionLinearInterpolant value(Interpolant::Array::array({0, 1}),
        Interpolant::Array::array({0, 0, 0, 1, 0, 1, 0, 0,
                                   0, 0, 1, 0, 1, 0, 0, 0}), 8);
    const auto result = value.evaluate(.5);
    REQUIRE(result.number(0) == Approx(std::sqrt(.5)));
    REQUIRE(result.number(1) == Approx(std::sqrt(.5)));
    REQUIRE(result.number(2) == 0);
    REQUIRE(result.number(3) == 0);
    REQUIRE(result.get(4).isUndefined());
    QuaternionLinearInterpolant missing(Interpolant::Array::array({0, 1}), Interpolant::Array::array({}), 4);
    REQUIRE(missing.evaluate(.5).get(0).isUndefined());
}
TEST_CASE("Bezier tangents share views and empty arrays differ from absent tangent data", "[interpolant]") {
    BezierInterpolant value(Interpolant::Array::array({0, 1}), Interpolant::Array::array({0, 1}), 1);
    REQUIRE(value.evaluate(.5).number(0) == .5);
    Float64Array out({1. / 3, 2, 1, 1}), in({0, 0, 2. / 3, 2});
    value.inTangents = in;
    value.outTangents = out;
    REQUIRE(value.evaluate(.5).number(0) == Approx(1.625));
    out[1] = 0;
    REQUIRE(value.evaluate(.5).number(0) == Approx(.875));
    value.inTangents = Interpolant::Array::array({});
    REQUIRE(std::isnan(value.evaluate(.5).number(0)));
    value.inTangents = nullptr;
    REQUIRE(value.evaluate(.5).number(0) == .5);
}

TEST_CASE("Discrete interval dispatch preserves timestamp objects without coercion", "[interpolant]") {
    auto counter = JSValue::object();
    counter.set("count", 0);
    auto end = numberWithCounter(counter, 1);
    DiscreteInterpolant value(Interpolant::Array::array({0, end}),
                               Interpolant::Array::array({5, 9}), 1);
    REQUIRE(value.evaluate(.5).number(0) == 5);
    REQUIRE(counter.get("count").number() == 1);
    REQUIRE(value.evaluate(.75).number(0) == 5);
    REQUIRE(counter.get("count").number() == 2);
}

TEST_CASE("Cubic snapshots stride before endpoint coercion and dispatches settings overrides", "[interpolant]") {
    auto previous = JSValue::object();
    previous.set("valueOf", JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                                  const std::vector<JSValue>&) {
        changingCubic->valueSize = 2;
        return JSValue(1);
    }));
    CubicInterpolant value(Interpolant::Array::array({0, previous, 2, 3, 4}),
                            Interpolant::Array::array({0, 10, 20, 30, 40, 50, 60, 70, 80, 90}), 1);
    changingCubic = &value;
    value.intervalChanged_(3, 2, 3);
    changingCubic = nullptr;
    REQUIRE(value._offsetPrev == 1);
    REQUIRE(value._offsetNext == 4);
    const auto result = value.interpolate_(3, 2, 2.5, 3);
    REQUIRE(result.number(0) == 53.125);
    REQUIRE(result.number(1) == 63.125);
    OverriddenSettings override(Interpolant::Array::array({0, 1, 3, 5}),
                                 Interpolant::Array::array({0, 10, -5, 20}), 1);
    override.overrideSettings.set("endingStart", ZeroSlopeEnding);
    REQUIRE(override.evaluate(.5).number(0) == 4.84375);
    REQUIRE(override.calls == 1);
}

TEST_CASE("Bezier reads control points before sample numeric conversion", "[interpolant]") {
    auto out = JSValue::fromJSON(JsonValue::Array{1. / 3, 0});
    auto first = JSValue::object();
    first.set("valueOf", JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                              const std::vector<JSValue>& captures) {
        auto out = captures[0];
        out.set("1", 100);
        return JSValue(0);
    }, {out}));
    BezierInterpolant value(Interpolant::Array::array({0, 1}),
                             Interpolant::Array::array({first, 8}), 1);
    value.outTangents = out;
    value.inTangents = Interpolant::Array::array({0, 0, 2. / 3, 0});
    REQUIRE(value.evaluate(.5).number(0) == 1);
    REQUIRE(out.get("1").number() == 100);
}

TEST_CASE("Quaternion flat interpolation repeats observable numeric conversions", "[interpolant]") {
    auto counter = JSValue::object();
    counter.set("count", 0);
    auto component = numberWithCounter(counter, 0, true);
    QuaternionLinearInterpolant value(Interpolant::Array::array({0, 1}),
        Interpolant::Array::array({component, 0, 0, 0, 0, 0, 0, 1}), 4);
    const auto result = value.evaluate(.5);
    REQUIRE(result.number(0) == Approx(1.414213562373095));
    REQUIRE(result.number(3) == Approx(.7071067811865475));
    REQUIRE(counter.get("count").number() == 2);
}
