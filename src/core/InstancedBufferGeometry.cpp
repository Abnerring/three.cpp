#include "core/InstancedBufferGeometry.h"

InstancedBufferGeometry::InstancedBufferGeometry() {
    isInstancedBufferGeometry = true;
    type = "InstancedBufferGeometry";
}
InstancedBufferGeometry& InstancedBufferGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    const auto* instanced = dynamic_cast<const InstancedBufferGeometry*>(&source);
    instanceCount = instanced ? instanced->instanceCount : JSValue();
    return *this;
}
std::shared_ptr<BufferGeometry> InstancedBufferGeometry::clone() const {
    auto result = std::make_shared<InstancedBufferGeometry>();
    result->copy(*this);
    return result;
}
JsonValue InstancedBufferGeometry::toJSON() const {
    auto result = BufferGeometry::toJSON();
    if (const auto count = instanceCount.toJSON()) result["instanceCount"] = *count;
    result["isInstancedBufferGeometry"] = true;
    return result;
}
