#pragma once
#include "core/JSMember.h"
#include <utility>

// CPU uniform tree, cache and upload protocol. Injected gl/textures objects
// implement the native GPU calls; no JavaScript engine is involved.
JSValue nativeWebGLUniformsConstructor();
class WebGLUniforms {
    JSValue object_;
    struct Existing {};
    static JSValue copy(const JSValue& value) {
        static_cast<void>(JSWeakValue(value));
        return value;
    }
    static JSValue take(JSValue& value) {
        static_cast<void>(JSWeakValue(value));
        return std::move(value);
    }
    WebGLUniforms(const JSValue& value, Existing) : object_(copy(value)) {}

  public:
    WebGLUniforms(const JSValue& gl = {}, const JSValue& program = {});
    WebGLUniforms(const WebGLUniforms& other) : object_(copy(other.object_)) {}
    WebGLUniforms(WebGLUniforms&& other) : object_(take(other.object_)) {}
    WebGLUniforms& operator=(const WebGLUniforms& other) {
        static_cast<void>(JSWeakValue(object_));
        object_ = copy(other.object_);
        return *this;
    }
    WebGLUniforms& operator=(WebGLUniforms&& other) {
        static_cast<void>(JSWeakValue(object_));
        if (this != &other)
            object_ = take(other.object_);
        return *this;
    }
    static WebGLUniforms fromObject(const JSValue& value) { return {value, Existing{}}; }
    // The wrapper and its JSMember views are confined to their owning thread.
    // Explicit entry points validate foreign handles before copying them.
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSMember seq{&object_, "seq"}, map{&object_, "map"};
    JSValue setValue(const JSValue& gl = {}, const JSValue& name = {}, const JSValue& value = {},
                     const JSValue& textures = {});
    JSValue setOptional(const JSValue& gl = {}, const JSValue& object = {},
                        const JSValue& name = {});
    static JSValue upload(const JSValue& gl = {}, const JSValue& seq = {},
                          const JSValue& values = {}, const JSValue& textures = {});
    static JSValue seqWithValue(const JSValue& seq = {}, const JSValue& values = {});
};
