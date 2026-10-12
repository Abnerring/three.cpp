#include "math/interpolants/DiscreteInterpolant.h"
#include "math/interpolants/LinearInterpolant.h"
#include "math/interpolants/CubicInterpolant.h"
#include "math/interpolants/QuaternionLinearInterpolant.h"
#include "math/interpolants/BezierInterpolant.h"
#include "constants.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using A = InterpolantArray;
const double quietNaN = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();
JsonValue value(const JSValue& v) {
    if (v.isUndefined()) return "$undefined";
    if (v.type() == JSValue::Type::Number) {
        const auto n = v.number();
        if (std::isnan(n)) return "$NaN";
        if (std::isinf(n)) return n < 0 ? "$-Infinity" : "$Infinity";
        if (n == 0 && std::signbit(n)) return "$-0";
        return n;
    }
    return v.toJSON().value_or(JsonValue("$undefined"));
}
JsonValue array(const A& a) {
    JsonValue::Array result;
    for (std::size_t i = 0; i < a.size(); ++i) result.push_back(value(a.get(double(i))));
    return result;
}
JsonValue run(Interpolant& interpolant, const std::vector<double>& times) {
    JsonValue::Array result;
    for (const auto t : times) {
        const auto sample = interpolant.evaluate(t);
        result.emplace_back(JsonValue::Object{{"value", array(sample)}, {"index", interpolant._cachedIndex},
                                              {"same", sample.sameIdentity(interpolant.resultBuffer)}});
    }
    return result;
}
class TraceInterpolant : public Interpolant {
  public:
    using Interpolant::Interpolant;
    JsonValue::Array calls;
    void intervalChanged_(std::ptrdiff_t i1, JSValue t0, JSValue t1) override {
        calls.emplace_back(JsonValue::Array{"interval", i1, value(t0), value(t1)});
    }
    Array interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) override {
        calls.emplace_back(JsonValue::Array{"interpolate", i1, value(t0), value(t), value(t1)});
        return copySampleValue_(i1 - 1);
    }
};
template <class C> JsonValue intervals() {
    C regular(A::array({0, 1, 2.5, 4, 8}), A::array({1, -1, 3, 2, -4, 7, 10, -2, 9, 5}), 2);
    JsonValue result = JsonValue::Object{};
    result["regular"] = run(regular, {-inf, -1, 0, .25, .75, 1, 2.2, 7, 3.5, .1, 8, inf, 4, quietNaN, 0});
    C duplicate(A::array({0, 1, 1, 2}), A::array({1, 5, 9, 13}), 1);
    result["duplicate"] = run(duplicate, {-1, 0, .5, 1, 1.5, 2, 1, quietNaN});
    C missing(A::array({0, JSValue{}, 2, 3}), A::array({1, 5, 9, 13}), 1);
    result["missing"] = run(missing, {.2, 2.5, .2, quietNaN, 3});
    C nonfinite(A::array({0, quietNaN, 2, 3}), A::array({1, 5, 9, 13}), 1);
    result["nanTime"] = run(nonfinite, {.2, 2.5, .2, quietNaN, 3});
    C empty(A::array({}), A::array({}), 2);
    result["empty"] = run(empty, {-1, 0, 1, quietNaN});
    C single(A::array({2}), A::array({7, -0.0}), 2);
    result["single"] = run(single, {-inf, 0, 2, 3, inf, quietNaN});
    C missingSamples(A::array({0, 1}), A::array({1, JSValue{}, quietNaN}), 2);
    result["missingSamples"] = run(missingSamples, {-1, .5, 1, 2});
    return result;
}
template <class T> JsonValue storage() {
    T values(4);
    values.set(std::vector<double>{-3, 257, 8, -129});
    Float64Array positions({0, 1});
    LinearInterpolant interpolant(positions, values, 2);
    JsonValue result = JsonValue::Object{{"samePositions", interpolant.parameterPositions.sameIdentity(positions)},
                                        {"sameValues", interpolant.sampleValues.sameIdentity(values)},
                                        {"initial", array(interpolant.resultBuffer)}};
    result["samples"] = run(interpolant, {-1, .25, .5, .75, 2});
    values[0] = 30;
    positions[1] = 2;
    result["mutated"] = run(interpolant, {.5, 1.5});
    return result;
}
void append(JSValue trace, const JSValue& entry) {
    trace.set(std::to_string(trace.length()), entry);
}
JSValue observedNumber(const JSValue& trace, const std::string& label, const JSValue& number) {
    auto result = JSValue::object();
    result.set("valueOf", JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                              const std::vector<JSValue>& captures) {
        append(captures[0], "number:" + captures[1].string());
        return captures[2];
    }, {trace, label, number}));
    return result;
}
JSValue observedArray(const JSValue& trace, const std::string& label,
                       const std::vector<JSValue>& entries) {
    auto result = JSValue::array(static_cast<std::uint32_t>(entries.size()));
    for (std::size_t i = 0; i < entries.size(); ++i) {
        JSPropertyDescriptor descriptor;
        descriptor.configurable = true;
        descriptor.enumerable = true;
        descriptor.getter = JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                                  const std::vector<JSValue>& captures) {
            append(captures[0], "get:" + captures[1].string());
            return captures[2];
        }, {trace, label + "." + std::to_string(i), entries[i]});
        result.defineProperty(std::to_string(i), descriptor);
    }
    return result;
}
template <class C> JsonValue parameterCallbacks(bool quaternion = false) {
    auto trace = JSValue::array();
    std::vector<JSValue> positions;
    for (const double t : {0., 1., 3., 5.})
        positions.push_back(observedNumber(trace, "time" + JSValue(t).toString(), t));
    const auto values = quaternion ? std::vector<JSValue>{0, 0, 0, 1, 0, 1, 0, 0,
                                                          0, 0, 1, 0, 1, 0, 0, 0}
                                   : std::vector<JSValue>{0, 10, -5, 20};
    C interpolant(observedArray(trace, "positions", positions),
                   observedArray(trace, "values", values), quaternion ? 4 : 1);
    const auto samples = run(interpolant, {.25, .75, 2, 4.5, .2, -1, 6, quietNaN, .5});
    return JsonValue::Object{{"samples", samples}, {"trace", array(trace)}};
}
CubicInterpolant* activeCubic = nullptr;
JsonValue cubicStrideCallback() {
    auto trace = JSValue::array();
    auto previous = JSValue::object();
    previous.set("valueOf", JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                                  const std::vector<JSValue>& captures) {
        append(captures[0], "stride=2");
        activeCubic->valueSize = 2;
        return JSValue(1);
    }, {trace}));
    CubicInterpolant interpolant(A::array({0, previous, 2, 3, 4}),
                                  A::array({0, 10, 20, 30, 40, 50, 60, 70, 80, 90}), 1);
    activeCubic = &interpolant;
    interpolant.intervalChanged_(3, 2, 3);
    const auto sample = array(interpolant.interpolate_(3, 2, 2.5, 3));
    activeCubic = nullptr;
    return JsonValue::Object{{"stride", interpolant.valueSize}, {"offsetPrev", interpolant._offsetPrev},
        {"offsetNext", interpolant._offsetNext}, {"sample", sample}, {"trace", array(trace)}};
}
class SettingsCallbacks : public CubicInterpolant {
  public:
    using CubicInterpolant::CubicInterpolant;
    JSValue trace = JSValue::array();
    JSValue getSettings_() const override {
        append(trace, "getSettings");
        return CubicInterpolant::getSettings_();
    }
};
JsonValue cubicSettingsCallback() {
    SettingsCallbacks interpolant(A::array({0, 1, 3, 5}), A::array({0, 10, -5, 20}), 1);
    interpolant.settings = JSValue::object();
    JSPropertyDescriptor descriptor;
    descriptor.getter = JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                              const std::vector<JSValue>& captures) {
        append(captures[0], "endingStart");
        const auto nested = array(activeCubic->evaluate(2));
        append(captures[0], JSValue::fromJSON(JsonValue::Array{"nested", nested, activeCubic->_cachedIndex}));
        return JSValue(ZeroSlopeEnding);
    }, {interpolant.trace});
    interpolant.settings.defineProperty("endingStart", descriptor);
    activeCubic = &interpolant;
    const auto samples = run(interpolant, {.5, .75, 4.5});
    activeCubic = nullptr;
    return JsonValue::Object{{"samples", samples}, {"trace", array(interpolant.trace)}};
}
JsonValue bezierReadCallback() {
    auto trace = JSValue::array();
    auto out = JSValue::fromJSON(JsonValue::Array{1. / 3, 0});
    auto first = JSValue::object();
    first.set("valueOf", JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                              const std::vector<JSValue>& captures) {
        append(captures[0], "sample");
        auto out = captures[1];
        out.set("1", 100);
        return JSValue(0);
    }, {trace, out}));
    BezierInterpolant interpolant(A::array({0, 1}), A::array({first, 8}), 1);
    interpolant.outTangents = out;
    interpolant.inTangents = A::array({0, 0, 2. / 3, 0});
    const auto sample = array(interpolant.evaluate(.5));
    return JsonValue::Object{{"sample", sample}, {"out", array(out)}, {"trace", array(trace)}};
}
JsonValue bezierArithmeticCallbacks() {
    auto trace = JSValue::array();
    const auto number = [&](const std::string& name, double v) { return observedNumber(trace, name, v); };
    BezierInterpolant interpolant(A::array({0, 1}),
        observedArray(trace, "values", {number("v0", 2), number("v1", 8)}), 1);
    interpolant.outTangents = observedArray(trace, "out", {number("c0x", .1), number("c0y", 3)});
    interpolant.inTangents = observedArray(trace, "in", {0, 0, number("c1x", .8), number("c1y", 6)});
    const auto sample = array(interpolant.evaluate(.35));
    return JsonValue::Object{{"sample", sample}, {"trace", array(trace)}};
}
JsonValue quaternionArithmeticCallbacks(double lastW, bool changing) {
    auto trace = JSValue::array();
    std::vector<JSValue> values;
    const std::vector<double> numbers{1, 0, 0, 0, 0, 0, 0, lastW};
    for (std::size_t i = 0; i < numbers.size(); ++i)
        values.push_back(observedNumber(trace, "q" + std::to_string(i), numbers[i]));
    if (changing) {
        auto count = JSValue::object();
        count.set("n", 0);
        values[0].set("valueOf", JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                                       const std::vector<JSValue>& captures) {
            append(captures[0], "number:q0");
            auto count = captures[1];
            const auto next = count.get("n").number() + 1;
            count.set("n", next);
            return JSValue(next);
        }, {trace, count}));
    }
    QuaternionLinearInterpolant interpolant(A::array({0, 1}), observedArray(trace, "values", values), 4);
    const auto sample = array(interpolant.evaluate(.5));
    return JsonValue::Object{{"sample", sample}, {"trace", array(trace)}};
}
JsonValue quaternionNegativeCallbacks() {
    auto trace = JSValue::array();
    std::vector<JSValue> values;
    const std::vector<double> numbers{0, 0, 0, 1, .001, 0, 0, -.9999995};
    for (std::size_t i = 0; i < numbers.size(); ++i)
        values.push_back(observedNumber(trace, "q" + std::to_string(i), numbers[i]));
    QuaternionLinearInterpolant interpolant(A::array({0, 1}), observedArray(trace, "values", values), 4);
    const auto sample = array(interpolant.evaluate(.5));
    return JsonValue::Object{{"sample", sample}, {"trace", array(trace)}};
}
JsonValue resultSetterCallback() {
    auto trace = JSValue::array();
    auto samples = JSValue::fromJSON(JsonValue::Array{1, 2, 3, 4});
    auto result = JSValue::array(2);
    auto stored = JSValue::object();
    JSPropertyDescriptor descriptor;
    descriptor.getter = JSValue::function([](const JSValue&, const std::vector<JSValue>&,
                                              const std::vector<JSValue>& captures) {
        return captures[0].get("value");
    }, {stored});
    descriptor.setter = JSValue::function([](const JSValue&, const std::vector<JSValue>& args,
                                              const std::vector<JSValue>& captures) {
        append(captures[0], args[0]);
        auto stored = captures[1], samples = captures[2];
        stored.set("value", args[0]);
        samples.set("1", 20);
        return JSValue{};
    }, {trace, stored, samples});
    result.defineProperty("0", descriptor);
    LinearInterpolant interpolant(A::array({0, 1}), samples, 2, result);
    const auto sample = array(interpolant.evaluate(.5));
    return JsonValue::Object{{"sample", sample}, {"samples", array(samples)}, {"trace", array(trace)}};
}
}
int main() {
    JsonValue output = JsonValue::Object{};
    TraceInterpolant trace(A::array({11, 22, 33, 44, 55, 66, 77, 88, 99}),
                           A::array({1, 2, 3, 4, 5, 6, 7, 8, 9}), 1);
    output["trace"] = run(trace, {11, 12, 22, 21, 20, 43, 12, 77, 80, 36, 24, 16, 100, 100, 89, -1, 11, quietNaN});
    output["calls"] = trace.calls;
    output["discrete"] = intervals<DiscreteInterpolant>();
    output["linear"] = intervals<LinearInterpolant>();
    output["cubic"] = intervals<CubicInterpolant>();
    output["bezierFallback"] = intervals<BezierInterpolant>();
    output["int8"] = storage<Int8Array>();
    output["uint8"] = storage<Uint8Array>();
    output["int16"] = storage<Int16Array>();
    output["uint16"] = storage<Uint16Array>();
    output["int32"] = storage<Int32Array>();
    output["uint32"] = storage<Uint32Array>();
    output["float32"] = storage<Float32Array>();
    output["float64"] = storage<Float64Array>();
    output["clamped"] = storage<Uint8ClampedArray>();
    auto ordinaryResult = JSValue::array();
    LinearInterpolant supplied(A::array({0, 1}), A::array({1, 2, 3, 4}), 2, ordinaryResult);
    output["suppliedIdentity"] = supplied.evaluate(.25).sameIdentity(ordinaryResult);
    output["supplied"] = array(ordinaryResult);
    LinearInterpolant shortResult(A::array({0, 1}), A::array({1, 2, 3, 4}), 2, Float32Array(1));
    output["shortTyped"] = run(shortResult, {.5, -1, 2});
    auto aliasValues = Float64Array({1, 2, 3, 4, 5, 6});
    LinearInterpolant alias(A::array({0, 1}), aliasValues, 3, aliasValues.subarray(1, 4));
    output["aliasedLinear"] = run(alias, {.5, -1, .25});
    output["aliasedValues"] = array(aliasValues);
    DiscreteInterpolant raw(nullptr, A::array({JSValue{}, quietNaN, nullptr, "7", true, -0.0}), 2, JSValue::array());
    JsonValue::Array rawCopies;
    for (const int index : {-1, 0, 1, 2, 3}) rawCopies.push_back(array(raw.copySampleValue_(index)));
    output["rawCopies"] = rawCopies;
    Interpolant base(A::array({0, 1}), A::array({5, 9}), 1);
    try { base.evaluate(.5); output["abstract"] = "no error"; }
    catch (const std::runtime_error& error) { output["abstract"] = error.what(); }
    output["baseBoundaries"] = run(base, {-1, 2});
    CubicInterpolant settings(A::array({0, 1, 3, 5}), A::array({0, 10, -5, 20}), 1);
    JsonValue::Array endings;
    for (const int mode : {ZeroCurvatureEnding, ZeroSlopeEnding, WrapAroundEnding, 999}) {
        settings.settings = JSValue::object();
        settings.settings.set("endingStart", mode);
        settings.settings.set("endingEnd", mode);
        settings._cachedIndex = 0;
        endings.push_back(run(settings, {.2, .6, 4.5, 4.8, .1}));
    }
    output["endings"] = endings;
    auto prototype = JSValue::object();
    prototype.set("endingStart", ZeroSlopeEnding);
    prototype.set("endingEnd", WrapAroundEnding);
    settings.settings = JSValue::object(prototype);
    settings._cachedIndex = 0;
    JsonValue::Array dynamic;
    dynamic.push_back(array(settings.evaluate(.25)));
    prototype.set("endingStart", WrapAroundEnding);
    dynamic.push_back(array(settings.evaluate(.5)));
    dynamic.push_back(array(settings.evaluate(2)));
    dynamic.push_back(array(settings.evaluate(.5)));
    output["dynamicSettings"] = dynamic;
    output["settingsIdentity"] = settings.getSettings_().sameValue(settings.settings);
    settings.settings = false;
    output["defaultIdentity"] = settings.getSettings_().sameValue(settings.DefaultSettings_);
    QuaternionLinearInterpolant quaternion(A::array({0, 1, 2}),
        A::array({0, 0, 0, 1, 0, .7071067811865476, 0, .7071067811865476, 0, 0, 0, -1}), 4);
    output["quaternion"] = run(quaternion, {0, .25, .5, .99, 1, 1.5, 2, quietNaN, .1});
    QuaternionLinearInterpolant small(A::array({0, 1}), A::array({0, 0, 0, 1, .001, 0, 0, .9999995}), 4);
    output["quaternionSmall"] = run(small, {.1, .5, .9});
    auto qValues = Float64Array({0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0});
    QuaternionLinearInterpolant multi(A::array({0, 1}), qValues, 8, qValues.subarray(4, 12));
    output["quaternionMulti"] = run(multi, {.5, .25});
    output["quaternionMultiValues"] = array(qValues);
    QuaternionLinearInterpolant undefinedQuaternion(A::array({0, 1}), A::array({}), 4);
    output["quaternionUndefined"] = run(undefinedQuaternion, {.5});
    BezierInterpolant bezier(A::array({0, 1, 3}), A::array({0, 10, 1, -5, 4, 2}), 2);
    auto tangentIn = Float64Array({0, 0, 0, 10, .9, 0, .8, 0, 2.7, 3, 2.2, 4});
    auto tangentOut = Float64Array({.2, 2, .3, 15, 1.3, -1, 1.8, -7, 3, 4, 3, 2});
    bezier.inTangents = tangentIn;
    bezier.outTangents = tangentOut;
    output["bezier"] = run(bezier, {0, .1, .4, .8, 1, 1.2, 2.4, 3, .5});
    tangentOut[1] = 8;
    output["bezierMutated"] = run(bezier, {.5});
    bezier.inTangents = A::array({});
    output["bezierMissing"] = run(bezier, {.5});
    bezier.inTangents = nullptr;
    output["bezierNullFallback"] = run(bezier, {.5});
    LinearInterpolant fractional(Float32Array({0, 1}), Float32Array({0, 1, 2, 3}), 1.5);
    output["fractionalConstructor"] = JsonValue::Array{fractional.valueSize, fractional.resultBuffer.size()};
    output["discreteCallbacks"] = parameterCallbacks<DiscreteInterpolant>();
    output["linearCallbacks"] = parameterCallbacks<LinearInterpolant>();
    output["cubicCallbacks"] = parameterCallbacks<CubicInterpolant>();
    output["quaternionCallbacks"] = parameterCallbacks<QuaternionLinearInterpolant>(true);
    output["bezierFallbackCallbacks"] = parameterCallbacks<BezierInterpolant>();
    output["cubicStrideCallback"] = cubicStrideCallback();
    output["cubicSettingsCallback"] = cubicSettingsCallback();
    CubicInterpolant strings(A::array({0, "1", "3", 5}), A::array({0, 10, -5, 20}), 1);
    strings.settings = JSValue::object();
    strings.settings.set("endingStart", WrapAroundEnding);
    strings.settings.set("endingEnd", WrapAroundEnding);
    output["cubicStringTimes"] = run(strings, {4, .25, 4.5, 2});
    output["bezierReadCallback"] = bezierReadCallback();
    output["bezierArithmeticCallbacks"] = bezierArithmeticCallbacks();
    output["quaternionChangingCallback"] = quaternionArithmeticCallbacks(1, true);
    output["quaternionNegativeCallbacks"] = quaternionNegativeCallbacks();
    output["resultSetterCallback"] = resultSetterCallback();
    std::cout << output.dump() << '\n';
}
