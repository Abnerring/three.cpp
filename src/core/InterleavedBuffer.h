#pragma once
#include "core/JSMember.h"
#include "core/NativeObject.h"

// Managed value handle: property writes, callback receivers and shared buffers
// use the same object graph as the JavaScript source.
class InterleavedBuffer {
  public:
    explicit InterleavedBuffer(JSValue array = {}, JSValue stride = {});
    template<class T, bool C>
    InterleavedBuffer(const TypedArray<T,C>& array, JSValue stride)
        : InterleavedBuffer(nativeArray(AnimationArray(array)), std::move(stride)) {}
    InterleavedBuffer(const InterleavedBuffer& other) : object_(other.object_) {}
    InterleavedBuffer(InterleavedBuffer&& other) noexcept : object_(std::move(other.object_)) {}
    InterleavedBuffer& operator=(const InterleavedBuffer& other) { object_=other.object_; return *this; }
    InterleavedBuffer& operator=(InterleavedBuffer&& other) noexcept { object_=std::move(other.object_); return *this; }
    static InterleavedBuffer fromObject(JSValue object) { return InterleavedBuffer(std::move(object), Existing{}); }
    static JSValue prototype();
    static JSValue constructor();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue get(const std::string& key) const { return object_.get(key); }
    void setProperty(const std::string& key, JSValue value) { object_.set(key,value); }
    JSMember isInterleavedBuffer{&object_,"isInterleavedBuffer"};
    JSMember array{&object_,"array"}, stride{&object_,"stride"}, count{&object_,"count"};
    JSMember usage{&object_,"usage"}, updateRanges{&object_,"updateRanges"}, version{&object_,"version"};
    JSMember uuid{&object_,"uuid"}, needsUpdate{&object_,"needsUpdate"}, onUploadCallback{&object_,"onUploadCallback"};
    JSValue setUsage(JSValue value);
    JSValue addUpdateRange(JSValue start, JSValue count);
    JSValue clearUpdateRanges();
    JSValue copy(JSValue source);
    JSValue copyAt(JSValue index1, JSValue source, JSValue index2);
    JSValue set(JSValue value, JSValue offset = 0);
    template<class T, bool C>
    JSValue set(const TypedArray<T,C>& value, JSValue offset = 0) { return set(nativeArray(AnimationArray(value)),offset); }
    JSValue clone(JSValue data = {});
    JSValue onUpload(JSValue callback);
    JSValue toJSON(JSValue data = {});
  protected:
    struct Existing {};
    InterleavedBuffer(JSValue value, Existing) : object_(std::move(value)) {}
    static void initialize(JSValue self, JSValue array, JSValue stride);
    JSValue object_;
};
