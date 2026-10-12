#include "catch.hpp"
#include "animation/PropertyMixer.h"
#include "math/interpolants/LinearInterpolant.h"
#include <limits>

namespace {
struct Binding final : PropertyBindingInterface {
    AnimationArray value;
    std::vector<double> gets, sets;
    std::function<void(AnimationArray, double)> onGet, onSet;
    explicit Binding(AnimationArray initial) : value(initial) {}
    void getValue(AnimationArray target, double offset) override {
        gets.push_back(offset);
        for (std::size_t i = 0; i < value.size(); ++i) target.set(offset + i, value.get(double(i)));
        const auto callback = onGet; if (callback) callback(target, offset);
    }
    void setValue(AnimationArray source, double offset) override {
        sets.push_back(offset);
        for (std::size_t i = 0; i < value.size(); ++i) value.set(double(i), source.get(offset + i));
        const auto callback = onSet; if (callback) callback(source, offset);
    }
};
}
TEST_CASE("PropertyMixer uses shared Float64 interpolant results and alternate accumulators", "[animation][property-mixer]") {
    auto binding = std::make_shared<Binding>(AnimationArray::array({10, 20}));
    PropertyMixer mixer(binding, "vector", 2);
    REQUIRE(std::holds_alternative<Float64Array>(mixer.buffer.storage()));
    REQUIRE(mixer.buffer.size() == 10);
    LinearInterpolant interpolation(AnimationArray::array({0, 1}), AnimationArray::array({0, 2, 4, 6}), 2, mixer.buffer);
    REQUIRE(interpolation.resultBuffer.sameIdentity(mixer.buffer));
    mixer.saveOriginalState();
    interpolation.evaluate(.5); mixer.accumulate(0, .5); mixer.apply(0);
    REQUIRE(binding->value.number(0) == 6);
    REQUIRE(binding->value.number(1) == 12);
    interpolation.evaluate(.5); mixer.accumulate(1, .5); mixer.apply(1);
    REQUIRE(binding->sets.size() == 1);
    mixer.restoreOriginalState();
    REQUIRE(binding->value.number(0) == 10);
    REQUIRE(binding->value.number(1) == 20);
    REQUIRE(binding->gets == std::vector<double>{6});
    REQUIRE(binding->sets == std::vector<double>{2, 6});
}
TEST_CASE("PropertyMixer applies additive numeric contributions and resets weights before binding callbacks", "[animation][property-mixer]") {
    auto binding = std::make_shared<Binding>(AnimationArray::array({4}));
    PropertyMixer mixer(binding, "number", 1); mixer.saveOriginalState();
    mixer.buffer.set(0, 8); mixer.accumulate(0, .25);
    mixer.buffer.set(0, 2); mixer.accumulateAdditive(.5);
    mixer.buffer.set(0, 6); mixer.accumulateAdditive(.25);
    bool zero = false;
    binding->onSet = [&](AnimationArray, double) {
        zero = mixer.cumulativeWeight.strictEquals(0) && mixer.cumulativeWeightAdditive.strictEquals(0);
        mixer.cumulativeWeight = 7;
    };
    mixer.apply(0);
    REQUIRE(binding->value.number(0) == 7.5);
    REQUIRE(zero);
    REQUIRE(mixer.cumulativeWeight.strictEquals(7));
    REQUIRE(mixer.cumulativeWeightAdditive.strictEquals(0));
}
TEST_CASE("PropertyMixer ordinary buffers retain holes and shared selected object identity", "[animation][property-mixer]") {
    auto original = JSValue::object(), selected = JSValue::object();
    auto binding = std::make_shared<Binding>(AnimationArray::array({original}));
    PropertyMixer mixer(binding, "string", 1);
    auto storage = std::get<JSValue>(mixer.buffer.storage());
    REQUIRE(storage.length() == 5);
    REQUIRE(storage.keys().empty());
    mixer.saveOriginalState();
    REQUIRE(storage.get("4").sameValue(original));
    mixer.buffer.set(0, selected); mixer.accumulate(0, .5); mixer.apply(0);
    REQUIRE(binding->sets.empty()); // The original wins the exact 0.5 selection boundary.
    mixer.buffer.set(0, selected); mixer.accumulate(1, .500001); mixer.apply(1);
    REQUIRE(binding->value.get(0).sameValue(selected));
    REQUIRE(binding->sets.size() == 1);
}
TEST_CASE("PropertyMixer binding replacement retains active receivers and original buffers", "[animation][property-mixer]") {
    auto initial = std::make_shared<Binding>(AnimationArray::array({10}));
    auto replacement = std::make_shared<Binding>(AnimationArray::array({90}));
    PropertyMixer mixer(initial, "number", 1);
    const auto old = mixer.buffer;
    initial->onGet = [&](AnimationArray argument, double) {
        REQUIRE(argument.sameIdentity(old));
        mixer.buffer = Float64Array({1, 2, 3, 4, 5});
        mixer.binding = replacement;
    };
    mixer.saveOriginalState();
    REQUIRE(old.number(1) == 10);
    REQUIRE(old.number(2) == 10);
    REQUIRE(mixer.buffer.number(1) == 2);
    REQUIRE(mixer.buffer.number(4) == 0);
    mixer.restoreOriginalState();
    REQUIRE(replacement->value.number(0) == 4);
    REQUIRE(initial->sets.empty());
    REQUIRE(replacement->sets == std::vector<double>{3});
}
TEST_CASE("PropertyMixer quaternion mixing keeps the original single quaternion stride behavior", "[animation][property-mixer]") {
    auto binding = std::make_shared<Binding>(AnimationArray::array({0, 0, 0, 1}));
    PropertyMixer mixer(binding, "quaternion", 4); mixer.saveOriginalState();
    REQUIRE(mixer.buffer.size() == 24);
    REQUIRE(mixer._workIndex.strictEquals(5));
    REQUIRE(mixer.buffer.number(19) == 1);
    mixer.buffer.set(0, 0); mixer.buffer.set(1, 1); mixer.buffer.set(2, 0); mixer.buffer.set(3, 0);
    mixer.accumulateAdditive(.5); mixer.apply(0);
    REQUIRE(binding->value.number(1) == Approx(std::sqrt(.5)));
    REQUIRE(binding->value.number(3) == Approx(std::sqrt(.5)));
    mixer.restoreOriginalState();
    REQUIRE(binding->value.number(3) == 1);
}
TEST_CASE("PropertyMixer preserves zero and NaN constructor behavior without eager loop rejection", "[animation][property-mixer]") {
    PropertyMixer empty(nullptr, "number", 0);
    empty.accumulate(0, 1); empty.apply(0);
    REQUIRE(empty.buffer.size() == 0);
    REQUIRE(empty.cumulativeWeight.strictEquals(0));
    REQUIRE_THROWS_AS(empty.restoreOriginalState(), JSTypeError);
    PropertyMixer fractional(nullptr, "number", .2);
    REQUIRE(fractional.buffer.size() == 1);
    fractional._select(fractional.buffer, 0, 0, .1, .2); // Unentered loop remains allowed.
    REQUIRE_THROWS_AS(fractional.accumulate(0, 1), JSRangeError);
    PropertyMixer nan(nullptr, "number", std::numeric_limits<double>::quiet_NaN());
    REQUIRE(nan.buffer.size() == 0);
    REQUIRE_THROWS_AS(PropertyMixer(nullptr, "string", std::numeric_limits<double>::quiet_NaN()), JSRangeError);
}
TEST_CASE("PropertyMixer quaternion equivalent frames do not emit a rounding-induced binding update", "[animation][property-mixer]") {
    auto binding = std::make_shared<Binding>(AnimationArray::array({0, 0, 0, 1}));
    PropertyMixer mixer(binding, "quaternion", 4); mixer.saveOriginalState();
    const auto incoming = [&](std::initializer_list<JSValue> values) {
        double i = 0; for (const auto& value : values) mixer.buffer.set(i++, value);
    };
    incoming({0, 1, 0, 0}); mixer.accumulate(0, .25);
    incoming({0, 0, 0, 1}); mixer.accumulate(0, .25); mixer.apply(0);
    REQUIRE(binding->sets.size() == 1);
    incoming({0, 1, 0, 0}); mixer.accumulateAdditive(.25); mixer.apply(1);
    REQUIRE(binding->sets.size() == 1);
    REQUIRE(mixer.buffer.get(5).strictEquals(mixer.buffer.get(9)));
    REQUIRE(mixer.buffer.get(7).strictEquals(mixer.buffer.get(11)));
}
TEST_CASE("PropertyMixer fractional comparisons can break before an unreachable endpoint", "[animation][property-mixer]") {
    auto binding = std::make_shared<Binding>(AnimationArray::array({1}));
    PropertyMixer mixer(binding, "number", .5);
    mixer.cumulativeWeight = 1;
    mixer.apply(0);
    REQUIRE(binding->sets == std::vector<double>{.5});
    mixer.buffer = AnimationArray::array({JSValue{}, JSValue{}, 4});
    mixer.cumulativeWeight = 1;
    mixer.apply(0); // The first comparison is equal; the second differs.
    REQUIRE(binding->sets == std::vector<double>{.5, .5});
    REQUIRE(binding->value.get(0).isUndefined());
}
TEST_CASE("PropertyMixer callbacks share mutable state and survive replacing themselves", "[animation][property-mixer]") {
    PropertyMixer mixer(nullptr, "number", 1), other(nullptr, "number", 1);
    REQUIRE(mixer._mixBufferRegion.sameIdentity(other._mixBufferRegion));
    std::vector<int> trace;
    mixer._mixBufferRegion = [&trace, count = 0](PropertyMixer& m, AnimationArray, double, double, JSValue, double) mutable {
        trace.push_back(++count);
        if (count == 2) m._mixBufferRegion = [&trace](PropertyMixer&, AnimationArray, double, double, JSValue, double) { trace.push_back(99); };
        trace.push_back(-count);
    };
    const auto copied = mixer._mixBufferRegion;
    mixer.cumulativeWeight = 1;
    for (int i = 0; i < 3; ++i) mixer.accumulate(0, 1);
    copied(mixer, mixer.buffer, 1, 0, .5, 1);
    REQUIRE(trace == std::vector<int>{1, -1, 2, -2, 99, 3, -3});
    REQUIRE_FALSE(copied.sameIdentity(mixer._mixBufferRegion));
    trace.clear();
    mixer._setIdentity = [&trace, count = 0](PropertyMixer& m) mutable {
        trace.push_back(++count);
        if (count == 2) m._setIdentity = [&trace](PropertyMixer&) { trace.push_back(99); };
        trace.push_back(-count);
    };
    for (int i = 0; i < 3; ++i) { mixer.cumulativeWeightAdditive = 0; mixer.accumulateAdditive(1); }
    REQUIRE(trace == std::vector<int>{1, -1, 2, -2, 99});
}
