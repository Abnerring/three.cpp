#pragma once
#include "core/JSValue.h"
#include <utility>

JSValue nativeWebGLShaderCacheConstructor();

// The pinned three.js class uses native managed Map/Set instances. Stages are
// ordinary objects with the source class prototype and shared module counter.
class WebGLShaderCache {
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
    WebGLShaderCache(const JSValue& value, Existing) : object_(copy(value)) {}

  public:
    WebGLShaderCache();
    WebGLShaderCache(const WebGLShaderCache& other) : object_(copy(other.object_)) {}
    WebGLShaderCache(WebGLShaderCache&& other) : object_(take(other.object_)) {}
    WebGLShaderCache& operator=(const WebGLShaderCache& other) {
        static_cast<void>(JSWeakValue(object_));
        object_ = copy(other.object_);
        return *this;
    }
    WebGLShaderCache& operator=(WebGLShaderCache&& other) {
        static_cast<void>(JSWeakValue(object_));
        if (this != &other) object_ = take(other.object_);
        return *this;
    }
    static WebGLShaderCache fromObject(const JSValue& value) { return {value, Existing{}}; }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSValue update(const JSValue& material = {}, const JSValue& vertexShaderStage = {},
                   const JSValue& fragmentShaderStage = {});
    JSValue remove(const JSValue& material = {});
    JSValue getVertexShaderStage(const JSValue& material = {});
    JSValue getFragmentShaderStage(const JSValue& material = {});
    JSValue dispose();
    JSValue _getShaderCacheForMaterial(const JSValue& material = {});
    JSValue _getShaderStage(const JSValue& code = {});
};
