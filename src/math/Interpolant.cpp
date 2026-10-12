#include "math/Interpolant.h"
#include <cmath>
#include <limits>

double Interpolant_detail::subtract(const JSValue& left, const JSValue& right) {
    const auto a = left.toNumber();
    const auto b = right.toNumber();
    return a - b;
}
double Interpolant_detail::multiply(const JSValue& left, const JSValue& right) {
    const auto a = left.toNumber();
    const auto b = right.toNumber();
    return a * b;
}
JSValue Interpolant_detail::add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive();
    const auto b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    return a.toNumber() + b.toNumber();
}

InterpolantArray InterpolantArray::array(std::initializer_list<JSValue> values) {
    auto result = JSValue::array(static_cast<std::uint32_t>(values.size()));
    std::size_t index = 0;
    for (const auto& value : values)
        result.set(std::to_string(index++), value);
    return result;
}
JSValue InterpolantArray::get(double index) const {
    return std::visit([&](const auto& values) -> JSValue {
        if constexpr (std::is_same_v<std::decay_t<decltype(values)>, JSValue>)
            return values.get(JSValue(index).toString());
        else
            return values.get(index);
    }, storage_);
}
void InterpolantArray::set(double index, const JSValue& value) {
    std::visit([&](auto& values) {
        if constexpr (std::is_same_v<std::decay_t<decltype(values)>, JSValue>)
            values.set(JSValue(index).toString(), value);
        else
            values.setIndex(index, value.toNumber());
    }, storage_);
}
std::size_t InterpolantArray::size() const {
    return std::visit([](const auto& values) -> std::size_t {
        if constexpr (std::is_same_v<std::decay_t<decltype(values)>, JSValue>)
            return values.length();
        else
            return values.size();
    }, storage_);
}
bool InterpolantArray::isUndefined() const {
    const auto* value = std::get_if<JSValue>(&storage_);
    return value && value->isUndefined();
}
bool InterpolantArray::toBoolean() const {
    const auto* value = std::get_if<JSValue>(&storage_);
    return !value || value->toBoolean();
}
bool InterpolantArray::sameIdentity(const InterpolantArray& other) const {
    if (storage_.index() != other.storage_.index())
        return false;
    return std::visit([&](const auto& values) {
        const auto& otherValues = std::get<std::decay_t<decltype(values)>>(other.storage_);
        if constexpr (std::is_same_v<std::decay_t<decltype(values)>, JSValue>)
            return values.sameValue(otherValues);
        else
            return values.sameIdentity(otherValues);
    }, storage_);
}
InterpolantArray InterpolantArray::allocate(double length) const {
    return std::visit([&](const auto& values) -> InterpolantArray {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, JSValue>) {
            if (values.type() != JSValue::Type::Array)
                throw JSTypeError("Interpolant sampleValues must be an Array or TypedArray");
            if (!std::isfinite(length) || length < 0 || length > UINT32_MAX ||
                std::trunc(length) != length)
                throw JSRangeError("Invalid array length");
            return JSValue::array(static_cast<std::uint32_t>(length));
        } else
            return T(length);
    }, storage_);
}

Interpolant::Interpolant(Array positions, Array values, double sampleSize, Array result)
    : parameterPositions(std::move(positions)),
      resultBuffer(result.isUndefined() ? values.allocate(sampleSize) : std::move(result)),
      sampleValues(std::move(values)), valueSize(sampleSize) {}

Interpolant::Array Interpolant::evaluate(double t) {
    const auto pp = parameterPositions;
    auto i1 = _cachedIndex;
    auto t1 = pp.get(double(i1));
    auto t0 = pp.get(double(i1) - 1);
    std::ptrdiff_t right = 0;
    // Labels preserve the original nested scan exits, including undefined/NaN
    // distinctions and the exact occasions on which intervalChanged_ is called.
    if (!(t < t1.toNumber())) {
        const auto giveUpAt = i1 + 2;
        for (;;) {
            if (t1.isUndefined()) {
                if (t < t0.toNumber())
                    goto reverse_scan;
                i1 = static_cast<std::ptrdiff_t>(pp.size());
                _cachedIndex = i1;
                return dispatchCopySampleValue_(i1 - 1);
            }
            if (i1 == giveUpAt)
                break;
            t0 = t1;
            t1 = pp.get(double(++i1));
            if (t < t1.toNumber())
                goto interval_found;
        }
        right = static_cast<std::ptrdiff_t>(pp.size());
        goto binary_search;
    }
reverse_scan:
    if (!(t >= t0.toNumber())) {
        const auto t1global = pp.get(1);
        if (t < t1global.toNumber()) {
            i1 = 2;
            t0 = t1global;
        }
        const auto giveUpAt = i1 - 2;
        for (;;) {
            if (t0.isUndefined()) {
                _cachedIndex = 0;
                return dispatchCopySampleValue_(0);
            }
            if (i1 == giveUpAt)
                break;
            t1 = t0;
            t0 = pp.get(double(--i1) - 1);
            if (t >= t0.toNumber())
                goto interval_found;
        }
        right = i1;
        i1 = 0;
        goto binary_search;
    }
    return dispatchInterpolate_(i1, t0, t, t1);
binary_search:
    while (i1 < right) {
        const auto mid = static_cast<std::uint32_t>(i1 + right) >> 1;
        if (t < pp.number(mid))
            right = mid;
        else
            i1 = mid + 1;
    }
    t1 = pp.get(double(i1));
    t0 = pp.get(double(i1) - 1);
    if (t0.isUndefined()) {
        _cachedIndex = 0;
        return dispatchCopySampleValue_(0);
    }
    if (t1.isUndefined()) {
        i1 = static_cast<std::ptrdiff_t>(pp.size());
        _cachedIndex = i1;
        return dispatchCopySampleValue_(i1 - 1);
    }
interval_found:
    _cachedIndex = i1;
    dispatchIntervalChanged_(i1, t0, t1);
    return dispatchInterpolate_(i1, t0, t, t1);
}
Interpolant::Array Interpolant::dispatchCopySampleValue_(std::ptrdiff_t index) {
    return runtimeMethods ? runtimeMethods->copySampleValue(runtimeReceiver,index) : copySampleValue_(index);
}
Interpolant::Array Interpolant::dispatchInterpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1) {
    return runtimeMethods ? runtimeMethods->interpolate(runtimeReceiver,i1,t0,t,t1) : interpolate_(i1,t0,t,t1);
}
void Interpolant::dispatchIntervalChanged_(std::ptrdiff_t i1, JSValue t0, JSValue t1) {
    if (runtimeMethods) runtimeMethods->intervalChanged(runtimeReceiver,i1,t0,t1);
    else intervalChanged_(i1,t0,t1);
}
JSValue Interpolant::dispatchGetSettings_() const {
    return runtimeMethods ? runtimeMethods->getSettings(runtimeReceiver) : getSettings_();
}
JSValue Interpolant::getSettings_() const {
    return settings.toBoolean() ? settings : DefaultSettings_;
}
Interpolant::Array Interpolant::copySampleValue_(std::ptrdiff_t index) {
    auto result = resultBuffer;
    const auto values = sampleValues;
    const auto stride = valueSize;
    requireFiniteStride(stride);
    const auto offset = double(index) * double(stride);
    for (double i = 0; i != stride; ++i)
        result.set(double(i), values.get(offset + double(i)));
    return result;
}
Interpolant::Array Interpolant::interpolate_(std::ptrdiff_t, JSValue, double, JSValue) {
    throw std::runtime_error("THREE.Interpolant: Call to abstract method.");
}
void Interpolant::intervalChanged_(std::ptrdiff_t, JSValue, JSValue) {}
void Interpolant::requireFiniteStride(double stride) {
    if (!std::isfinite(stride) || stride < 0 || std::trunc(stride) != stride || stride > 9007199254740991.0)
        throw JSRangeError("Interpolant sample size would cause a nonterminating loop");
}
