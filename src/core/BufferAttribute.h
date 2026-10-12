#pragma once

#include "constants.h"
#include "core/EventDispatcher.h"
#include "core/JSMember.h"
#include "core/BufferAttributeRuntime.h"
#include "core/TypedArray.h"
#include "core/VertexAttribute.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

/** Typed-array storage and normalized component semantics from three.js BufferAttribute. */
class BufferAttribute : public EventDispatcher, public VertexAttribute, public std::enable_shared_from_this<BufferAttribute> {
    std::shared_ptr<BufferAttributeStorage> storage_;
    JSValue object_;
    friend void initializeBufferAttributeRuntime(BufferAttribute&);
  public:
    using Array = BufferAttributeArray;
    const bool isBufferAttribute = true;
    JSMember isFloat16BufferAttribute{&object_, "isFloat16BufferAttribute"};
    const std::uint64_t& id;
    std::string& name;
    Array& array;
    double& itemSize;
    double& count;
    bool& normalized;
    JSMember usage{&object_, "usage"}, gpuType{&object_, "gpuType"};
    JSMember updateRanges{&object_, "updateRanges"}, version{&object_, "version"};
    JSMember needsUpdate{&object_, "needsUpdate"}, onUploadCallback{&object_, "onUploadCallback"};
    // The same managed object is authoritative before and after nativeAttribute().
    JSValue object() const;
    const JSValue* managedObjectRoot() const override { return &object_; }
    const std::shared_ptr<BufferAttributeStorage>& runtimeStorage() const { return storage_; }
    static std::shared_ptr<BufferAttribute> restore(std::shared_ptr<BufferAttributeStorage> storage,JSValue object);

    template <class T>
    BufferAttribute(std::vector<T> values, double size, bool normalize = false)
        : BufferAttribute(TypedArray<T>(values), size, normalize) {}
    template <class T, bool Clamped>
    BufferAttribute(TypedArray<T, Clamped> values, double size, bool normalize = false)
        : storage_(std::make_shared<BufferAttributeStorage>(Array(std::move(values)),size,normalize)),
          id(storage_->id),name(storage_->name),array(storage_->array),itemSize(storage_->itemSize),count(storage_->count),normalized(storage_->normalized) {
        initializeBufferAttributeRuntime(*this);
    }
    ~BufferAttribute() override = default;
    JSValue getProperty(const std::string& name) const override;
    InterpolantArray getArray() const override {
        return std::visit([](const auto& values)->InterpolantArray {return values;},array);
    }
    void setNeedsUpdate(JSValue value) override { needsUpdate=value; }
    std::shared_ptr<VertexAttribute> cloneAttribute(JSValue = {}) const override { return clone(); }
    JSValue serializeAttribute(JSValue = {}) const override { return JSValue::fromJSON(toJSON()); }
    BufferAttribute(const BufferAttribute&) = delete;
    BufferAttribute& operator=(const BufferAttribute&) = delete;

    JSValue setUsage(JSValue value);
    JSValue addUpdateRange(JSValue start, JSValue count);
    JSValue clearUpdateRanges();
    JSValue onUpload(JSValue callback);
    JSValue onUpload(std::function<void()> callback);
    JSValue onUpload(std::nullptr_t) { return onUpload(JSValue(nullptr)); }
    // Explicit managed function invocation; callers may retain the function value.
    JSValue invokeUploadCallback() const;
    virtual std::shared_ptr<BufferAttribute> clone() const;
    virtual BufferAttribute& copy(const BufferAttribute& source);
    BufferAttribute& copyAt(double destination, const BufferAttribute& source,
                            double sourceIndex);
    template <class T> BufferAttribute& set(const std::vector<T>& values, double offset = 0) {
        std::visit([&](auto& destination) { destination.set(values, offset); }, array);
        return *this;
    }
    template <class T> BufferAttribute& copyArray(const std::vector<T>& values) {
        return set(values);
    }
    template <class T, bool C>
    BufferAttribute& set(const TypedArray<T, C>& values, double offset = 0) {
        std::visit([&](auto& destination) { destination.set(values, offset); }, array);
        return *this;
    }
    template <class T, bool C> BufferAttribute& copyArray(const TypedArray<T, C>& values) {
        return set(values);
    }
    double getComponent(double index, double component) const;
    BufferAttribute& setComponent(double index, double component, double value);
    virtual double getX(double index) const { return getComponent(index, 0); }
    virtual double getY(double index) const { return getComponent(index, 1); }
    virtual double getZ(double index) const { return getComponent(index, 2); }
    virtual double getW(double index) const { return getComponent(index, 3); }
    virtual BufferAttribute& setX(double i, double x) { return setComponent(i, 0, x); }
    virtual BufferAttribute& setY(double i, double y) { return setComponent(i, 1, y); }
    virtual BufferAttribute& setZ(double i, double z) { return setComponent(i, 2, z); }
    virtual BufferAttribute& setW(double i, double w) { return setComponent(i, 3, w); }
    virtual BufferAttribute& setXY(double i, double x, double y) {
        setComponent(i, 0, x);
        return setComponent(i, 1, y);
    }
    virtual BufferAttribute& setXYZ(double i, double x, double y, double z) {
        setComponent(i, 0, x);
        setComponent(i, 1, y);
        return setComponent(i, 2, z);
    }
    virtual BufferAttribute& setXYZW(double i, double x, double y, double z, double w) {
        setComponent(i, 0, x);
        setComponent(i, 1, y);
        setComponent(i, 2, z);
        return setComponent(i, 3, w);
    }
    BufferAttribute& applyMatrix3(const Matrix3& matrix);
    BufferAttribute& applyMatrix4(const Matrix4& matrix) override;
    BufferAttribute& applyNormalMatrix(const Matrix3& matrix) override;
    BufferAttribute& transformDirection(const Matrix4& matrix) override;
    virtual JsonValue toJSON() const;
    std::string arrayType() const;
    void dispose() { dispatchEvent(Event{"dispose"}); }

  protected:
    struct RuntimeTag {};
    BufferAttribute(std::shared_ptr<BufferAttributeStorage> storage,JSValue object,RuntimeTag)
        :storage_(std::move(storage)),object_(std::move(object)),id(storage_->id),name(storage_->name),array(storage_->array),itemSize(storage_->itemSize),count(storage_->count),normalized(storage_->normalized) {}
    void setRuntimeFactory(BufferAttributeFactory factory) { storage_->factory=factory; }
  private:
    double getRaw(double offset) const;
    void setRaw(double offset, double value);
};

template <class T, bool Clamped = false> class TypedBufferAttribute : public BufferAttribute {
    TypedBufferAttribute(std::shared_ptr<BufferAttributeStorage> storage,JSValue object,RuntimeTag tag)
        :BufferAttribute(std::move(storage),std::move(object),tag) {}
    void preserveRuntimeType() { setRuntimeFactory(+[](std::shared_ptr<BufferAttributeStorage> storage,JSValue object)->std::shared_ptr<BufferAttribute> {
        return std::shared_ptr<BufferAttribute>(new TypedBufferAttribute(std::move(storage),std::move(object),RuntimeTag{}));
    }); }
  public:
    TypedBufferAttribute(std::initializer_list<T> values, double itemSize, bool normalized = false)
        : BufferAttribute(TypedArray<T, Clamped>(values), itemSize, normalized) { preserveRuntimeType(); }
    TypedBufferAttribute(ArrayBuffer values, double itemSize, bool normalized = false)
        : BufferAttribute(TypedArray<T, Clamped>(std::move(values)), itemSize, normalized) { preserveRuntimeType(); }
    TypedBufferAttribute(double length, double itemSize, bool normalized = false)
        : BufferAttribute(TypedArray<T, Clamped>(length), itemSize, normalized) { preserveRuntimeType(); }
    TypedBufferAttribute(std::vector<T> values, double itemSize, bool normalized = false)
        : BufferAttribute(TypedArray<T, Clamped>(values), itemSize, normalized) { preserveRuntimeType(); }
    template <class U, bool C>
    TypedBufferAttribute(const TypedArray<U, C>& values, double itemSize, bool normalized = false)
        : BufferAttribute(TypedArray<T, Clamped>(values.size()), itemSize, normalized) {
        preserveRuntimeType();
        std::get<TypedArray<T, Clamped>>(array).set(values);
    }
    template <class U, std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<T, U>, int> = 0>
    TypedBufferAttribute(const std::vector<U>& values, double itemSize, bool normalized = false)
        : BufferAttribute(TypedArray<T, Clamped>(values), itemSize, normalized) { preserveRuntimeType(); }
    std::shared_ptr<BufferAttribute> clone() const override {
        auto result =
            std::make_shared<TypedBufferAttribute<T, Clamped>>(std::vector<T>{}, itemSize, normalized);
        result->copy(*this);
        return result;
    }
};
using Int8BufferAttribute = TypedBufferAttribute<std::int8_t>;
using Uint8BufferAttribute = TypedBufferAttribute<std::uint8_t>;
using Uint8ClampedBufferAttribute = TypedBufferAttribute<std::uint8_t, true>;
using Int16BufferAttribute = TypedBufferAttribute<std::int16_t>;
using Uint16BufferAttribute = TypedBufferAttribute<std::uint16_t>;
using Int32BufferAttribute = TypedBufferAttribute<std::int32_t>;
using Uint32BufferAttribute = TypedBufferAttribute<std::uint32_t>;
using Float32BufferAttribute = TypedBufferAttribute<float>;
using Float64BufferAttribute = TypedBufferAttribute<double>;

class Float16BufferAttribute : public BufferAttribute {
    Float16BufferAttribute(std::shared_ptr<BufferAttributeStorage> storage,JSValue object,RuntimeTag tag)
        :BufferAttribute(std::move(storage),std::move(object),tag) {}
    void preserveRuntimeType() { setRuntimeFactory(+[](std::shared_ptr<BufferAttributeStorage> storage,JSValue object)->std::shared_ptr<BufferAttribute> {
        return std::shared_ptr<BufferAttribute>(new Float16BufferAttribute(std::move(storage),std::move(object),RuntimeTag{}));
    }); }
  public:
    Float16BufferAttribute(std::initializer_list<std::uint16_t> values, double size, bool normalize = false)
        : BufferAttribute(Uint16Array(values), size, normalize) { preserveRuntimeType(); isFloat16BufferAttribute = true; }
    Float16BufferAttribute(ArrayBuffer values, double size, bool normalize = false)
        : BufferAttribute(Uint16Array(std::move(values)), size, normalize) { preserveRuntimeType(); isFloat16BufferAttribute = true; }
    Float16BufferAttribute(std::vector<std::uint16_t> values, double size, bool normalize = false)
        : BufferAttribute(Uint16Array(values), size, normalize) { preserveRuntimeType(); isFloat16BufferAttribute = true; }
    template <class T> Float16BufferAttribute(const std::vector<T>& values, double size, bool normalize = false)
        : BufferAttribute(Uint16Array(values), size, normalize) { preserveRuntimeType(); isFloat16BufferAttribute = true; }
    template <class T, bool C> Float16BufferAttribute(const TypedArray<T, C>& values, double size, bool normalize = false)
        : BufferAttribute(Uint16Array(values.size()), size, normalize) {
        preserveRuntimeType(); isFloat16BufferAttribute = true;
        std::get<Uint16Array>(array).set(values);
    }
    Float16BufferAttribute(double length, double size, bool normalize = false)
        : BufferAttribute(Uint16Array(length), size, normalize) { preserveRuntimeType(); isFloat16BufferAttribute = true; }
    std::shared_ptr<BufferAttribute> clone() const override;
    double getX(double i) const override { return getHalf(i * itemSize); }
    double getY(double i) const override { return getHalf(i * itemSize + 1); }
    double getZ(double i) const override { return getHalf(i * itemSize + 2); }
    double getW(double i) const override { return getHalf(i * itemSize + 3); }
    Float16BufferAttribute& setX(double i, double x) override { return setHalf(i * itemSize, x); }
    Float16BufferAttribute& setY(double i, double y) override { return setHalf(i * itemSize + 1, y); }
    Float16BufferAttribute& setZ(double i, double z) override { return setHalf(i * itemSize + 2, z); }
    Float16BufferAttribute& setW(double i, double w) override { return setHalf(i * itemSize + 3, w); }
    Float16BufferAttribute& setXY(double i, double x, double y) override;
    Float16BufferAttribute& setXYZ(double i, double x, double y, double z) override;
    Float16BufferAttribute& setXYZW(double i, double x, double y, double z, double w) override;
  private:
    double getHalf(double offset) const;
    Float16BufferAttribute& setHalf(double offset, double value);
};
