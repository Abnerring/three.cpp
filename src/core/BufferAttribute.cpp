#include "core/BufferAttribute.h"
#include "core/JSDataSnapshot.h"
#include "extras/DataUtils.h"
#include "math/MathUtils.h"
#include "math/Vector2.h"
#include "math/Vector3.h"

JSValue BufferAttribute::getProperty(const std::string& property) const {
    if(property=="name") return name;
    if(property=="itemSize") return itemSize;
    if(property=="count") return count;
    if(property=="normalized") return normalized;
    if(property=="isBufferAttribute") return isBufferAttribute;
    if(property=="isFloat16BufferAttribute" || property=="usage" || property=="gpuType" ||
       property=="version" || property=="updateRanges" || property=="onUploadCallback" ||
       property=="clearUpdateRanges" || property=="needsUpdate") return object_.get(property);
    return {};
}

JSValue BufferAttribute::object() const {
    if (const auto owner = weak_from_this().lock())
        bindBufferAttributeRuntime(object_, std::const_pointer_cast<BufferAttribute>(owner));
    return object_;
}
std::shared_ptr<BufferAttribute> BufferAttribute::restore(std::shared_ptr<BufferAttributeStorage> storage,JSValue object) {
    auto result=storage->factory?storage->factory(storage,object):std::shared_ptr<BufferAttribute>(new BufferAttribute(storage,object,RuntimeTag{}));
    bindBufferAttributeRuntime(object,result);return result;
}
JSValue BufferAttribute::setUsage(JSValue value) {
    const auto self=object(), method=self.get("setUsage"); return method.call(self,{value});
}
JSValue BufferAttribute::addUpdateRange(JSValue start, JSValue count) {
    const auto self=object(), method=self.get("addUpdateRange"); return method.call(self,{start,count});
}
JSValue BufferAttribute::clearUpdateRanges() {
    const auto self=object(), method=self.get("clearUpdateRanges"); return method.call(self);
}
JSValue BufferAttribute::onUpload(JSValue callback) {
    const auto self=object(), method=self.get("onUpload"); return method.call(self,{callback});
}
JSValue BufferAttribute::onUpload(std::function<void()> callback) {
    if (!callback) return onUpload(JSValue(nullptr));
    // Native callback captures are opaque C++ ownership. Managed callbacks use
    // the JSValue overload with explicit captures so their cycles are traceable.
    auto holder=std::make_shared<std::function<void()>>(std::move(callback));
    const auto host=JSValue::hostObject(holder,holder.get(),"three.BufferAttributeCallback");
    return onUpload(JSValue::function(+[](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue {
        const auto target=std::static_pointer_cast<std::function<void()>>(captures[0].hostPayload("three.BufferAttributeCallback"));
        (*target)(); return {};
    },{host}));
}
JSValue BufferAttribute::invokeUploadCallback() const {
    const auto self=object(), callback=self.get("onUploadCallback"); return callback.call(self);
}

namespace {
thread_local Vector2 attributeVector2;
thread_local Vector3 attributeVector3;
double denormalizeComponent(double value, const BufferAttribute::Array& array) {
    return std::visit([&](const auto& values) -> double {
        using T = typename std::decay_t<decltype(values)>::value_type;
        if constexpr (std::is_same_v<T, double>)
            throw std::invalid_argument("THREE.MathUtils: Invalid component type.");
        else if constexpr (std::is_integral_v<T>) {
            value /= static_cast<double>(std::numeric_limits<T>::max());
            if constexpr (std::is_signed_v<T>) value = MathUtils::max(value, -1);
        }
        return value;
    }, array);
}
double normalizeComponent(double value, const BufferAttribute::Array& array) {
    return std::visit([&](const auto& values) -> double {
        using T = typename std::decay_t<decltype(values)>::value_type;
        if constexpr (std::is_same_v<T, double>)
            throw std::invalid_argument("THREE.MathUtils: Invalid component type.");
        else if constexpr (std::is_integral_v<T>)
            value = MathUtils::round(value * static_cast<double>(std::numeric_limits<T>::max()));
        return value;
    }, array);
}
}

std::shared_ptr<BufferAttribute> Float16BufferAttribute::clone() const {
    auto result = std::make_shared<Float16BufferAttribute>(0.0, itemSize, normalized);
    result->copy(*this);
    return result;
}
double Float16BufferAttribute::getHalf(double offset) const {
    double value = std::visit([&](const auto& values) {
        return fromHalfFloat(values.get(offset).toNumber());
    }, array);
    if (normalized) value = denormalizeComponent(value, array);
    return value;
}
Float16BufferAttribute& Float16BufferAttribute::setHalf(double offset, double value) {
    if (normalized) value = normalizeComponent(value, array);
    const auto half = toHalfFloat(value);
    std::visit([&](auto& values) { values.setIndex(offset, half); }, array);
    return *this;
}
Float16BufferAttribute& Float16BufferAttribute::setXY(double i, double x, double y) {
    i *= itemSize;
    setHalf(i, x); setHalf(i + 1, y);
    return *this;
}
Float16BufferAttribute& Float16BufferAttribute::setXYZ(double i, double x, double y, double z) {
    i *= itemSize;
    setHalf(i, x); setHalf(i + 1, y); setHalf(i + 2, z);
    return *this;
}
Float16BufferAttribute& Float16BufferAttribute::setXYZW(double i, double x, double y, double z, double w) {
    i *= itemSize;
    setHalf(i, x); setHalf(i + 1, y); setHalf(i + 2, z); setHalf(i + 3, w);
    return *this;
}

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
    array = std::visit([](const auto& values) -> Array { return values.slice(); }, source.array);
    itemSize = source.itemSize;
    count = source.count;
    normalized = source.normalized;
    usage = source.usage.value();
    gpuType = source.gpuType.value();
    return *this;
}

double BufferAttribute::getRaw(double offset) const {
    return std::visit(
        [&](const auto& a) {
            const auto value = a.get(offset);
            return value.isUndefined() ? std::numeric_limits<double>::quiet_NaN() : value.number();
        },
        array);
}
void BufferAttribute::setRaw(double offset, double value) {
    std::visit(
        [&](auto& a) {
            a.setIndex(offset, value);
        },
        array);
}
double BufferAttribute::getComponent(double index, double component) const {
    const auto value = getRaw(index * itemSize + component);
    return normalized ? denormalizeComponent(value, array) : value;
}
BufferAttribute& BufferAttribute::setComponent(double index, double component,
                                               double value) {
    if (normalized) value = normalizeComponent(value, array);
    setRaw(index * itemSize + component, value);
    return *this;
}
BufferAttribute& BufferAttribute::copyAt(double destination, const BufferAttribute& source,
                                         double sourceIndex) {
    destination *= itemSize;
    sourceIndex *= source.itemSize;
    for (double i = 0, limit = itemSize; i < limit; ++i)
        setRaw(destination + i, source.getRaw(sourceIndex + i));
    return *this;
}
BufferAttribute& BufferAttribute::applyMatrix3(const Matrix3& matrix) {
    if (itemSize == 2) {
        for (double i = 0, limit = count; i < limit; ++i) {
            attributeVector2.fromBufferAttribute(*this, i).applyMatrix3(matrix);
            setXY(i, attributeVector2.x, attributeVector2.y);
        }
    } else if (itemSize == 3) {
        for (double i = 0, limit = count; i < limit; ++i) {
            attributeVector3.fromBufferAttribute(*this, i).applyMatrix3(matrix);
            setXYZ(i, attributeVector3.x, attributeVector3.y, attributeVector3.z);
        }
    }
    return *this;
}
BufferAttribute& BufferAttribute::applyMatrix4(const Matrix4& matrix) {
    for (double i = 0, limit = count; i < limit; ++i) {
        attributeVector3.fromBufferAttribute(*this, i).applyMatrix4(matrix);
        setXYZ(i, attributeVector3.x, attributeVector3.y, attributeVector3.z);
    }
    return *this;
}
BufferAttribute& BufferAttribute::applyNormalMatrix(const Matrix3& matrix) {
    for (double i = 0, limit = count; i < limit; ++i) {
        attributeVector3.fromBufferAttribute(*this, i).applyNormalMatrix(matrix);
        setXYZ(i, attributeVector3.x, attributeVector3.y, attributeVector3.z);
    }
    return *this;
}
BufferAttribute& BufferAttribute::transformDirection(const Matrix4& matrix) {
    for (double i = 0, limit = count; i < limit; ++i) {
        attributeVector3.fromBufferAttribute(*this, i).transformDirection(matrix);
        setXYZ(i, attributeVector3.x, attributeVector3.y, attributeVector3.z);
    }
    return *this;
}
std::string BufferAttribute::arrayType() const {
    static const char* names[]{"Int8Array",  "Uint8Array",  "Int16Array",   "Uint16Array",
                               "Int32Array", "Uint32Array", "Float32Array", "Float64Array",
                               "Uint8ClampedArray"};
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
    JsonValue result=JsonValue::Object{
        {"itemSize", itemSize}, {"type", arrayType()}, {"array", std::move(values)},
        {"normalized", normalized}, {"name", name}};
    if(const auto value=snapshotData(usage.value())) result["usage"]=*value;
    if(const auto value=snapshotData(gpuType.value())) result["gpuType"]=*value;
    return result;
}
