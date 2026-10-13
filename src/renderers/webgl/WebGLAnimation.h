#pragma once
#include "core/JSValue.h"

// The context supplies native requestAnimationFrame/cancelAnimationFrame methods.
// This module preserves three.js scheduling; it does not create a browser host.
JSValue nativeWebGLAnimationConstructor();

class WebGLAnimation {
    JSValue object_;
    static JSValue copy(const JSValue& value) {
        static_cast<void>(JSWeakValue(value));
        return value;
    }
    static JSValue take(JSValue& value) {
        static_cast<void>(JSWeakValue(value));
        return std::move(value);
    }
  public:
    WebGLAnimation();
    WebGLAnimation(const WebGLAnimation& other) : object_(copy(other.object_)) {}
    WebGLAnimation(WebGLAnimation&& other) : object_(take(other.object_)) {}
    WebGLAnimation& operator=(const WebGLAnimation& other) {
        static_cast<void>(JSWeakValue(object_)); object_ = copy(other.object_); return *this;
    }
    WebGLAnimation& operator=(WebGLAnimation&& other) {
        static_cast<void>(JSWeakValue(object_));
        if (this != &other) object_ = take(other.object_);
        return *this;
    }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    JSValue start();
    JSValue stop();
    JSValue setAnimationLoop(const JSValue& callback = {});
    JSValue setContext(const JSValue& context = {});
};
