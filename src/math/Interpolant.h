#pragma once

#include "core/TypedArray.h"
#include <cstddef>
#include <optional>
#include <variant>

// Keep the conversion order of JavaScript arithmetic. In particular, callers
// must evaluate separate operands before combining them in a C++ expression.
namespace Interpolant_detail {
double subtract(const JSValue& left, const JSValue& right);
double multiply(const JSValue& left, const JSValue& right);
JSValue add(const JSValue& left, const JSValue& right);
}

// A shared array handle. Ordinary arrays retain holes/undefined; typed arrays
// retain their view identity and apply their storage conversion on every write.
class InterpolantArray {
  public:
    using Storage = std::variant<Int8Array, Uint8Array, Int16Array, Uint16Array,
                                 Int32Array, Uint32Array, Float32Array, Float64Array,
                                 Uint8ClampedArray, JSValue>;
    InterpolantArray() : storage_(JSValue{}) {}
    InterpolantArray(std::nullptr_t) : storage_(JSValue(nullptr)) {}
    InterpolantArray(JSValue value) : storage_(std::move(value)) {}
    template <class T, bool C>
    InterpolantArray(TypedArray<T, C> value) : storage_(std::move(value)) {}
    template <class T> InterpolantArray(const std::vector<T>& values)
        : storage_(JSValue::array(static_cast<std::uint32_t>(values.size()))) {
        auto& array = std::get<JSValue>(storage_);
        for (std::size_t i = 0; i < values.size(); ++i)
            array.set(std::to_string(i), JSValue(T(values[i])));
    }
    static InterpolantArray array(std::initializer_list<JSValue> values);
    JSValue get(double index) const;
    double number(double index) const { return get(index).toNumber(); }
    void set(double index, const JSValue& value);
    std::size_t size() const;
    bool isUndefined() const;
    bool toBoolean() const;
    bool sameIdentity(const InterpolantArray& other) const;
    InterpolantArray allocate(double length) const;
    const Storage& storage() const { return storage_; }
    Storage& storage() { return storage_; }

  private:
    Storage storage_;
};

class Interpolant {
  public:
    using Array = InterpolantArray;
    Array parameterPositions;
    std::ptrdiff_t _cachedIndex = 0;
    JSValue __cacheIndex;
    Array resultBuffer;
    Array sampleValues;
    double valueSize;
    JSValue settings = nullptr;
    JSValue DefaultSettings_ = JSValue::object();
    // The runtime adapter owns one explicit, enumerable GC root. The dispatch
    // table contains only function pointers; callbacks stay in the JS graph.
    struct RuntimeMethods {
        Array (*copySampleValue)(JSValue receiver, std::ptrdiff_t index);
        Array (*interpolate)(JSValue receiver, std::ptrdiff_t i1, JSValue t0, double t, JSValue t1);
        void (*intervalChanged)(JSValue receiver, std::ptrdiff_t i1, JSValue t0, JSValue t1);
        JSValue (*getSettings)(JSValue receiver);
    };
    JSValue runtimeReceiver;
    const RuntimeMethods* runtimeMethods = nullptr;

    Interpolant(Array parameterPositions, Array sampleValues, double sampleSize,
                Array resultBuffer = {});
    virtual ~Interpolant() = default;
    virtual Array evaluate(double t);
    virtual JSValue getSettings_() const;
    virtual Array copySampleValue_(std::ptrdiff_t index);
    virtual Array interpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1);
    virtual void intervalChanged_(std::ptrdiff_t i1, JSValue t0, JSValue t1);
  protected:
    Array dispatchCopySampleValue_(std::ptrdiff_t index);
    Array dispatchInterpolate_(std::ptrdiff_t i1, JSValue t0, double t, JSValue t1);
    void dispatchIntervalChanged_(std::ptrdiff_t i1, JSValue t0, JSValue t1);
    JSValue dispatchGetSettings_() const;
    // Fractional/negative/NaN strides make upstream's i !== stride loop nonterminating.
    static void requireFiniteStride(double stride);
};
