#pragma once

#include "constants.h"
#include "core/EventDispatcher.h"
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
class BufferAttribute : public EventDispatcher {
  public:
    using Array =
        std::variant<std::vector<std::int8_t>, std::vector<std::uint8_t>, std::vector<std::int16_t>,
                     std::vector<std::uint16_t>, std::vector<std::int32_t>,
                     std::vector<std::uint32_t>, std::vector<float>, std::vector<double>>;
    struct UpdateRange {
        std::size_t start, count;
    };
    struct UpdateFlag {
        std::uint64_t* version;
        void operator=(bool value) const {
            if (value)
                ++*version;
        }
    };

    const bool isBufferAttribute = true;
    const std::uint64_t id = nextId++;
    std::string name;
    Array array;
    std::size_t itemSize, count;
    bool normalized;
    int usage = StaticDrawUsage;
    int gpuType = FloatType;
    std::vector<UpdateRange> updateRanges;
    std::uint64_t version = 0;
    UpdateFlag needsUpdate{&version};
    std::function<void()> onUploadCallback = [] {};

    template <class T>
    BufferAttribute(std::vector<T> values, std::size_t size, bool normalize = false)
        : array(std::move(values)), itemSize(size), count(0), normalized(normalize) {
        if (!itemSize)
            throw std::invalid_argument("BufferAttribute.itemSize must be positive");
        count = std::visit([&](const auto& values) { return values.size() / itemSize; }, array);
    }
    virtual ~BufferAttribute() = default;
    BufferAttribute(const BufferAttribute&) = delete;
    BufferAttribute& operator=(const BufferAttribute&) = delete;

    BufferAttribute& setUsage(int value) {
        usage = value;
        return *this;
    }
    void addUpdateRange(std::size_t start, std::size_t length) {
        updateRanges.push_back({start, length});
    }
    void clearUpdateRanges() { updateRanges.clear(); }
    BufferAttribute& onUpload(std::function<void()> callback) {
        onUploadCallback = std::move(callback);
        return *this;
    }
    virtual std::shared_ptr<BufferAttribute> clone() const;
    BufferAttribute& copy(const BufferAttribute& source);
    BufferAttribute& copyAt(std::size_t destination, const BufferAttribute& source,
                            std::size_t sourceIndex);
    template <class T> BufferAttribute& set(const std::vector<T>& values, std::size_t offset = 0) {
        const auto length = std::visit([](const auto& a) { return a.size(); }, array);
        if (offset > length || values.size() > length - offset)
            throw std::out_of_range("BufferAttribute.set");
        for (std::size_t i = 0; i < values.size(); ++i)
            setRaw(offset + i, values[i]);
        return *this;
    }
    template <class T> BufferAttribute& copyArray(const std::vector<T>& values) {
        return set(values);
    }
    double getComponent(std::size_t index, std::size_t component) const;
    BufferAttribute& setComponent(std::size_t index, std::size_t component, double value);
    double getX(std::size_t index) const { return getComponent(index, 0); }
    double getY(std::size_t index) const { return getComponent(index, 1); }
    double getZ(std::size_t index) const { return getComponent(index, 2); }
    double getW(std::size_t index) const { return getComponent(index, 3); }
    BufferAttribute& setX(std::size_t i, double x) { return setComponent(i, 0, x); }
    BufferAttribute& setY(std::size_t i, double y) { return setComponent(i, 1, y); }
    BufferAttribute& setZ(std::size_t i, double z) { return setComponent(i, 2, z); }
    BufferAttribute& setW(std::size_t i, double w) { return setComponent(i, 3, w); }
    BufferAttribute& setXY(std::size_t i, double x, double y) {
        setX(i, x);
        return setY(i, y);
    }
    BufferAttribute& setXYZ(std::size_t i, double x, double y, double z) {
        setXY(i, x, y);
        return setZ(i, z);
    }
    BufferAttribute& setXYZW(std::size_t i, double x, double y, double z, double w) {
        setXYZ(i, x, y, z);
        return setW(i, w);
    }
    BufferAttribute& applyMatrix3(const Matrix3& matrix);
    BufferAttribute& applyMatrix4(const Matrix4& matrix);
    BufferAttribute& applyNormalMatrix(const Matrix3& matrix);
    BufferAttribute& transformDirection(const Matrix4& matrix);
    JsonValue toJSON() const;
    std::string arrayType() const;
    void dispose() { dispatchEvent(Event{"dispose"}); }

  private:
    inline static std::atomic<std::uint64_t> nextId{0};
    double getRaw(std::size_t offset) const;
    void setRaw(std::size_t offset, double value);
};

template <class T> class TypedBufferAttribute : public BufferAttribute {
  public:
    TypedBufferAttribute(std::vector<T> values, std::size_t itemSize, bool normalized = false)
        : BufferAttribute(std::move(values), itemSize, normalized) {}
    std::shared_ptr<BufferAttribute> clone() const override {
        auto result =
            std::make_shared<TypedBufferAttribute<T>>(std::vector<T>{}, itemSize, normalized);
        result->copy(*this);
        return result;
    }
};
using Int8BufferAttribute = TypedBufferAttribute<std::int8_t>;
using Uint8BufferAttribute = TypedBufferAttribute<std::uint8_t>;
using Int16BufferAttribute = TypedBufferAttribute<std::int16_t>;
using Uint16BufferAttribute = TypedBufferAttribute<std::uint16_t>;
using Int32BufferAttribute = TypedBufferAttribute<std::int32_t>;
using Uint32BufferAttribute = TypedBufferAttribute<std::uint32_t>;
using Float32BufferAttribute = TypedBufferAttribute<float>;
using Float64BufferAttribute = TypedBufferAttribute<double>;
