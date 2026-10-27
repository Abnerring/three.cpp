#pragma once
#include "math/Interpolant.h"

// ArrayBuffer and typed-array wrappers share their native storage control block.
// This keeps JS extension properties alive while native handles remain alive.
JSValue nativeArrayBuffer(const ArrayBuffer& buffer);
// Stable intrinsic binding, independent of prototype.constructor mutation.
JSValue nativeArrayBufferConstructor();
ArrayBuffer arrayBuffer(const JSValue& value);
struct NativeArrayOwnership {
    std::vector<JSNativeOwnerEdge> edges;
    std::vector<JSNativeOwnerNode> nodes;
};
#define THREE_NATIVE_ARRAY_OWNER(Type) NativeArrayOwnership nativeArrayOwnership(const Type& array);
THREE_NATIVE_ARRAY_OWNER(Int8Array) THREE_NATIVE_ARRAY_OWNER(Uint8Array)
THREE_NATIVE_ARRAY_OWNER(Int16Array) THREE_NATIVE_ARRAY_OWNER(Uint16Array)
THREE_NATIVE_ARRAY_OWNER(Int32Array) THREE_NATIVE_ARRAY_OWNER(Uint32Array)
THREE_NATIVE_ARRAY_OWNER(Float32Array) THREE_NATIVE_ARRAY_OWNER(Float64Array)
THREE_NATIVE_ARRAY_OWNER(Uint8ClampedArray)
#undef THREE_NATIVE_ARRAY_OWNER
namespace NativeArray_detail {
// Integer-indexed objects terminate lookup for canonical numeric keys, even
// when the index has no own descriptor. Proxies are handled by their target.
bool isNumericProperty(const JSValue& value, const std::string& key);
JSValue constructor(std::size_t kind);
JSValue wrap(const InterpolantArray& array);
InterpolantArray unwrap(const JSValue& value);
}
