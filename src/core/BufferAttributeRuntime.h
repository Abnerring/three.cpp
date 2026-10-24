#pragma once
#include "core/JSValue.h"
#include "core/TypedArray.h"
#include <atomic>
#include <memory>
#include <variant>

class BufferAttribute;
struct BufferAttributeStorage;
using BufferAttributeArray=std::variant<Int8Array,Uint8Array,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array,Uint8ClampedArray>;
using BufferAttributeFactory=std::shared_ptr<BufferAttribute>(*)(std::shared_ptr<BufferAttributeStorage>,JSValue);
struct BufferAttributeStorage {
    inline static std::atomic<std::uint64_t> nextId{0};
    const std::uint64_t id=nextId++;
    std::string name;
    BufferAttributeArray array;
    double itemSize,count;
    bool normalized;
    JSValue meshPerAttribute;
    BufferAttributeFactory factory=nullptr;
    BufferAttributeStorage(BufferAttributeArray values,double size,bool normalize)
        :array(std::move(values)),itemSize(size),count(std::visit([&](const auto& a){return double(a.size())/size;},array)),normalized(normalize) {}
};
// The wrapper owns storage, never the native handle. A surviving wrapper may
// recreate a handle sharing the exact same storage and managed identity.
struct BufferAttributeRuntime {
    std::shared_ptr<BufferAttributeStorage> storage;
    std::weak_ptr<BufferAttribute> owner;
};
void initializeBufferAttributeRuntime(BufferAttribute& attribute);
void initializeFloat16BufferAttributeRuntime(BufferAttribute& attribute);
void bindBufferAttributeRuntime(const JSValue& object,const std::shared_ptr<BufferAttribute>& attribute);
std::shared_ptr<BufferAttribute> bufferAttributeRuntimeValue(const JSValue& object);
