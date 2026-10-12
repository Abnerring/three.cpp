#pragma once
#include "core/BufferAttribute.h"

class InstancedBufferAttribute : public BufferAttribute {
    InstancedBufferAttribute(std::shared_ptr<BufferAttributeStorage> storage,JSValue object,RuntimeTag tag)
        :BufferAttribute(std::move(storage),std::move(object),tag),meshPerAttribute(runtimeStorage()->meshPerAttribute) {}
  public:
    const bool isInstancedBufferAttribute = true;
    JSValue& meshPerAttribute;
    template <class T, bool Clamped>
    InstancedBufferAttribute(TypedArray<T, Clamped> values, double itemSize,
                             bool normalized = false, JSValue meshPerAttribute = 1)
        : BufferAttribute(std::move(values), itemSize, normalized),
          meshPerAttribute(runtimeStorage()->meshPerAttribute) {
        this->meshPerAttribute=meshPerAttribute;
        setRuntimeFactory(+[](std::shared_ptr<BufferAttributeStorage> storage,JSValue object)->std::shared_ptr<BufferAttribute> {
            return std::shared_ptr<BufferAttribute>(new InstancedBufferAttribute(std::move(storage),std::move(object),RuntimeTag{}));
        });
    }
    template <class T>
    InstancedBufferAttribute(std::vector<T> values, double itemSize,
                             bool normalized = false, JSValue meshPerAttribute = 1)
        : InstancedBufferAttribute(TypedArray<T>(values), itemSize, normalized, meshPerAttribute) {}
    InstancedBufferAttribute& copy(const BufferAttribute& source) override;
    std::shared_ptr<BufferAttribute> clone() const override;
    JsonValue toJSON() const override;
};
