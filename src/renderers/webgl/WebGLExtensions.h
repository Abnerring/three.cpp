#pragma once
#include "core/JSMember.h"
#include <utility>

// Pinned three.js CPU protocol over an injected native GPU command object.
// This module does not determine or advertise Vulkan device capabilities.
JSValue nativeWebGLExtensionsConstructor();
class WebGLExtensions {
    JSValue object_;
    struct Existing {};
    static JSValue copy(const JSValue& value) { static_cast<void>(JSWeakValue(value)); return value; }
    static JSValue take(JSValue& value) { static_cast<void>(JSWeakValue(value)); return std::move(value); }
    WebGLExtensions(const JSValue& value, Existing) : object_(copy(value)) {}
  public:
    explicit WebGLExtensions(const JSValue& gl = {});
    WebGLExtensions(const WebGLExtensions& other) : object_(copy(other.object_)) {}
    WebGLExtensions(WebGLExtensions&& other) : object_(take(other.object_)) {}
    WebGLExtensions& operator=(const WebGLExtensions& other) { static_cast<void>(JSWeakValue(object_)); object_=copy(other.object_); return *this; }
    WebGLExtensions& operator=(WebGLExtensions&& other) { static_cast<void>(JSWeakValue(object_)); if(this!=&other)object_=take(other.object_); return *this; }
    static WebGLExtensions fromObject(const JSValue& value) { return {value,Existing{}}; }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSValue has(const JSValue& name = {});
    JSValue init();
    JSValue get(const JSValue& name = {});
    static void setConsoleFunction(const JSValue& callback);
    static JSValue getConsoleFunction();
};
