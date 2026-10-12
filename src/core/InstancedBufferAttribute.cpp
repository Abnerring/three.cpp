#include "core/InstancedBufferAttribute.h"

InstancedBufferAttribute& InstancedBufferAttribute::copy(const BufferAttribute& source) {
    BufferAttribute::copy(source);
    const auto* instanced = dynamic_cast<const InstancedBufferAttribute*>(&source);
    meshPerAttribute = instanced ? instanced->meshPerAttribute : JSValue();
    return *this;
}
std::shared_ptr<BufferAttribute> InstancedBufferAttribute::clone() const {
    auto result = std::make_shared<InstancedBufferAttribute>(Float32Array(), itemSize, normalized);
    result->copy(*this);
    return result;
}
JsonValue InstancedBufferAttribute::toJSON() const {
    auto data = BufferAttribute::toJSON();
    if (const auto serialized = meshPerAttribute.toJSON()) data["meshPerAttribute"] = *serialized;
    data["isInstancedBufferAttribute"] = true;
    return data;
}
