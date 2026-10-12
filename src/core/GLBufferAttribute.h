#pragma once
#include "core/JSMember.h"

// A managed handle to the upstream attribute object. Buffer is an opaque
// identity-bearing JSValue/native host resource; this class never reads CPU
// arrays or calls a graphics API. Vulkan resource binding remains to be added
// by the renderer; this managed class alone does not provide GPU binding.
class GLBufferAttribute {
    JSValue object_;
    struct Existing {};
    GLBufferAttribute(const JSValue& object, Existing);
  public:
    explicit GLBufferAttribute(const JSValue& buffer = {}, const JSValue& type = {},
                              const JSValue& itemSize = {}, const JSValue& elementSize = {},
                              const JSValue& count = {}, const JSValue& normalized = false);
    GLBufferAttribute(const GLBufferAttribute& other);
    GLBufferAttribute(GLBufferAttribute&& other) noexcept : object_(std::move(other.object_)) {}
    GLBufferAttribute& operator=(const GLBufferAttribute& other);
    GLBufferAttribute& operator=(GLBufferAttribute&& other) noexcept { object_=std::move(other.object_); return *this; }
    static GLBufferAttribute fromObject(const JSValue& object) { return GLBufferAttribute(object, Existing{}); }
    static JSValue constructor();
    static JSValue prototype();
    JSValue object() const;
    operator JSValue() const { return object(); }
    JSValue get(const std::string& key) const;
    void setProperty(const std::string& key, const JSValue& value);
    JSMember isGLBufferAttribute{&object_, "isGLBufferAttribute"};
    JSMember name{&object_, "name"}, buffer{&object_, "buffer"}, type{&object_, "type"};
    JSMember itemSize{&object_, "itemSize"}, elementSize{&object_, "elementSize"}, count{&object_, "count"};
    JSMember normalized{&object_, "normalized"}, version{&object_, "version"}, needsUpdate{&object_, "needsUpdate"};
    JSValue setBuffer(const JSValue& buffer = {});
    JSValue setType(const JSValue& type = {}, const JSValue& elementSize = {});
    JSValue setItemSize(const JSValue& itemSize = {});
    JSValue setCount(const JSValue& count = {});
};

JSValue nativeGLBufferAttributeConstructor();
JSValue nativeGLBufferAttribute(const GLBufferAttribute& attribute);
