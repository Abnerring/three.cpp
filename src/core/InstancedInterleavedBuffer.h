#pragma once
#include "core/InterleavedBuffer.h"

class InstancedInterleavedBuffer : public InterleavedBuffer {
  public:
    explicit InstancedInterleavedBuffer(JSValue array = {}, JSValue stride = {}, JSValue meshPerAttribute = 1);
    template<class T,bool C>
    InstancedInterleavedBuffer(const TypedArray<T,C>& array,JSValue stride,JSValue meshPerAttribute=1)
        : InstancedInterleavedBuffer(nativeArray(AnimationArray(array)),stride,meshPerAttribute) {}
    InstancedInterleavedBuffer(const InstancedInterleavedBuffer& other):InterleavedBuffer(other) {}
    InstancedInterleavedBuffer(InstancedInterleavedBuffer&& other) noexcept:InterleavedBuffer(std::move(other)) {}
    InstancedInterleavedBuffer& operator=(const InstancedInterleavedBuffer& other) { InterleavedBuffer::operator=(other); return *this; }
    InstancedInterleavedBuffer& operator=(InstancedInterleavedBuffer&& other) noexcept { InterleavedBuffer::operator=(std::move(other)); return *this; }
    static InstancedInterleavedBuffer fromObject(JSValue object) { return InstancedInterleavedBuffer(std::move(object),Existing{}); }
    static JSValue prototype();
    JSMember isInstancedInterleavedBuffer{&object_,"isInstancedInterleavedBuffer"};
    JSMember meshPerAttribute{&object_,"meshPerAttribute"};
  private:
    InstancedInterleavedBuffer(JSValue object,Existing):InterleavedBuffer(std::move(object),Existing{}) {}
    static void initializeInstance(JSValue self,JSValue array,JSValue stride,JSValue meshPerAttribute);
};
