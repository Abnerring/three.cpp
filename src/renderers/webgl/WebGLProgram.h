#pragma once
#include "core/JSValue.h"
#include <utility>

// The pinned source's shader construction/link/cache protocol, accepting a
// native rendering command object. This module does not manufacture GPU
// handles.
JSValue nativeWebGLProgramConstructor();
class WebGLProgram {
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
    WebGLProgram(const JSValue& value, Existing) : object_(copy(value)) {}

  public:
    WebGLProgram(const JSValue& renderer = {}, const JSValue& cacheKey = {},
                 const JSValue& parameters = {}, const JSValue& bindingStates = {});
    WebGLProgram(const WebGLProgram& other) : object_(copy(other.object_)) {}
    WebGLProgram(WebGLProgram&& other) : object_(take(other.object_)) {}
    WebGLProgram& operator=(const WebGLProgram& other) {
        static_cast<void>(JSWeakValue(object_));
        object_ = copy(other.object_);
        return *this;
    }
    WebGLProgram& operator=(WebGLProgram&& other) {
        static_cast<void>(JSWeakValue(object_));
        if (this != &other)
            object_ = take(other.object_);
        return *this;
    }
    static WebGLProgram fromObject(const JSValue& value) { return {value, Existing{}}; }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSValue getUniforms();
    JSValue getAttributes();
    JSValue isReady();
    JSValue destroy();
};
