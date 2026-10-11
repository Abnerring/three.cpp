#include "core/BufferAttribute.h"
#include "math/Vector3.h"

namespace {
template <class T> T typedValue(double value) {
    if constexpr (std::is_floating_point_v<T>)
        return static_cast<T>(value);
    else {
        // TypedArray writes use ToIntN/ToUintN, including modulo wraparound.
        if (!std::isfinite(value) || value == 0)
            return 0;
        constexpr double modulus = static_cast<double>(std::uint64_t{1} << (sizeof(T) * 8));
        auto wrapped = std::fmod(std::trunc(value), modulus);
        if (wrapped < 0)
            wrapped += modulus;
        if constexpr (std::is_signed_v<T>) {
            if (wrapped >= modulus / 2)
                wrapped -= modulus;
        }
        return static_cast<T>(wrapped);
    }
}
} // namespace

std::shared_ptr<BufferAttribute> BufferAttribute::clone() const {
    auto result = std::visit(
        [&](const auto& a) -> std::shared_ptr<BufferAttribute> {
            return std::make_shared<BufferAttribute>(a, itemSize, normalized);
        },
        array);
    result->copy(*this);
    return result;
}

BufferAttribute& BufferAttribute::copy(const BufferAttribute& source) {
    name = source.name;
    array = source.array;
    itemSize = source.itemSize;
    count = source.count;
    normalized = source.normalized;
    usage = source.usage;
    gpuType = source.gpuType;
    return *this;
}

double BufferAttribute::getRaw(std::size_t offset) const {
    return std::visit(
        [&](const auto& a) {
            return offset < a.size() ? static_cast<double>(a[offset])
                                     : std::numeric_limits<double>::quiet_NaN();
        },
        array);
}
void BufferAttribute::setRaw(std::size_t offset, double value) {
    std::visit(
        [&](auto& a) {
            using T = typename std::decay_t<decltype(a)>::value_type;
            if (offset < a.size())
                a[offset] = typedValue<T>(value);
        },
        array);
}
double BufferAttribute::getComponent(std::size_t index, std::size_t component) const {
    const auto maximum = std::numeric_limits<std::size_t>::max();
    const auto offset =
        index > (maximum - component) / itemSize ? maximum : index * itemSize + component;
    return std::visit(
        [&](const auto& a) -> double {
            using T = typename std::decay_t<decltype(a)>::value_type;
            double value = offset < a.size() ? static_cast<double>(a[offset])
                                             : std::numeric_limits<double>::quiet_NaN();
            if constexpr (std::is_same_v<T, double>) {
                if (normalized)
                    throw std::invalid_argument("THREE.MathUtils: Invalid component type.");
            }
            if constexpr (std::is_integral_v<T>) {
                if (normalized)
                    value =
                        std::max(value / static_cast<double>(std::numeric_limits<T>::max()), -1.0);
            }
            return value;
        },
        array);
}
BufferAttribute& BufferAttribute::setComponent(std::size_t index, std::size_t component,
                                               double value) {
    if (normalized)
        std::visit(
            [&](const auto& a) {
                using T = typename std::decay_t<decltype(a)>::value_type;
                if constexpr (std::is_same_v<T, double>)
                    throw std::invalid_argument("THREE.MathUtils: Invalid component type.");
                if constexpr (std::is_integral_v<T>)
                    value = std::floor(value * std::numeric_limits<T>::max() + 0.5);
            },
            array);
    const auto maximum = std::numeric_limits<std::size_t>::max();
    if (index <= (maximum - component) / itemSize)
        setRaw(index * itemSize + component, value);
    return *this;
}
BufferAttribute& BufferAttribute::copyAt(std::size_t destination, const BufferAttribute& source,
                                         std::size_t sourceIndex) {
    for (std::size_t i = 0; i < itemSize; ++i)
        setRaw(destination * itemSize + i, source.getRaw(sourceIndex * source.itemSize + i));
    return *this;
}
BufferAttribute& BufferAttribute::applyMatrix3(const Matrix3& matrix) {
    const auto e = matrix.elements();
    if (itemSize != 2 && itemSize != 3)
        return *this;
    for (std::size_t i = 0; i < count; ++i) {
        const double x = getX(i), y = getY(i), z = itemSize == 2 ? 1 : getZ(i);
        setXY(i, e[0] * x + e[3] * y + e[6] * z, e[1] * x + e[4] * y + e[7] * z);
        if (itemSize == 3)
            setZ(i, e[2] * x + e[5] * y + e[8] * z);
    }
    return *this;
}
BufferAttribute& BufferAttribute::applyMatrix4(const Matrix4& matrix) {
    for (std::size_t i = 0; i < count; ++i) {
        Vector3 v(getX(i), getY(i), getZ(i));
        v.applyMatrix4(matrix);
        setXYZ(i, v.x(), v.y(), v.z());
    }
    return *this;
}
BufferAttribute& BufferAttribute::applyNormalMatrix(const Matrix3& matrix) {
    const auto e = matrix.elements();
    for (std::size_t i = 0; i < count; ++i) {
        const double x = getX(i), y = getY(i), z = getZ(i);
        Vector3 v(e[0] * x + e[3] * y + e[6] * z, e[1] * x + e[4] * y + e[7] * z,
                  e[2] * x + e[5] * y + e[8] * z);
        v.normalize();
        setXYZ(i, v.x(), v.y(), v.z());
    }
    return *this;
}
BufferAttribute& BufferAttribute::transformDirection(const Matrix4& matrix) {
    for (std::size_t i = 0; i < count; ++i) {
        Vector3 v(getX(i), getY(i), getZ(i));
        v.transformDirection(matrix);
        setXYZ(i, v.x(), v.y(), v.z());
    }
    return *this;
}
std::string BufferAttribute::arrayType() const {
    static const char* names[]{"Int8Array",  "Uint8Array",  "Int16Array",   "Uint16Array",
                               "Int32Array", "Uint32Array", "Float32Array", "Float64Array"};
    return names[array.index()];
}
JsonValue BufferAttribute::toJSON() const {
    JsonValue::Array values;
    std::visit(
        [&](const auto& a) {
            for (auto v : a)
                values.emplace_back(v);
        },
        array);
    return JsonValue::Object{
        {"itemSize", itemSize},     {"type", arrayType()}, {"array", std::move(values)},
        {"normalized", normalized}, {"name", name},        {"usage", usage},
        {"gpuType", gpuType}};
}
