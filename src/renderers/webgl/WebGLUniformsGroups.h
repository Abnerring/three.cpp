#pragma once
#include "core/JSValue.h"
#include <utility>

// CPU STD140 packing, cache and native uniform-buffer command protocol.
// The injected command/state objects supply GPU operations; Vulkan integration
// and GPU output verification are separate from this source protocol port.
JSValue nativeWebGLUniformsGroupsConstructor();
class WebGLUniformsGroups {
    JSValue object_;
    struct Existing {};
    static JSValue copy(const JSValue& value) { static_cast<void>(JSWeakValue(value)); return value; }
    static JSValue take(JSValue& value) { static_cast<void>(JSWeakValue(value)); return std::move(value); }
    WebGLUniformsGroups(const JSValue& value, Existing) : object_(copy(value)) {}
  public:
    explicit WebGLUniformsGroups(const JSValue& gl = {}, const JSValue& info = {},
                                const JSValue& capabilities = {}, const JSValue& state = {});
    WebGLUniformsGroups(const WebGLUniformsGroups& other) : object_(copy(other.object_)) {}
    WebGLUniformsGroups(WebGLUniformsGroups&& other) : object_(take(other.object_)) {}
    WebGLUniformsGroups& operator=(const WebGLUniformsGroups& other) {
        static_cast<void>(JSWeakValue(object_)); object_ = copy(other.object_); return *this;
    }
    WebGLUniformsGroups& operator=(WebGLUniformsGroups&& other) {
        static_cast<void>(JSWeakValue(object_)); if (this != &other) object_ = take(other.object_); return *this;
    }
    static WebGLUniformsGroups fromObject(const JSValue& value) { return {value, Existing{}}; }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSValue bind(const JSValue& uniformsGroup = {}, const JSValue& program = {});
    JSValue update(const JSValue& uniformsGroup = {}, const JSValue& program = {});
    JSValue dispose();
};
