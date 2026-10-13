#pragma once
#include "core/JSMember.h"

// Pinned three.js CPU clipping-plane state and projection protocol.
// Shader clipping and Vulkan uniform binding are separate renderer work.
JSValue nativeWebGLClippingConstructor();
class WebGLClipping {
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
    WebGLClipping(const JSValue& value, Existing) : object_(copy(value)) {}

  public:
    explicit WebGLClipping(const JSValue& properties = {});
    WebGLClipping(const WebGLClipping& other) : object_(copy(other.object_)) {}
    WebGLClipping(WebGLClipping&& other) : object_(take(other.object_)) {}
    WebGLClipping& operator=(const WebGLClipping& other) {
        static_cast<void>(JSWeakValue(object_));
        object_ = copy(other.object_);
        return *this;
    }
    WebGLClipping& operator=(WebGLClipping&& other) {
        static_cast<void>(JSWeakValue(object_));
        if (this != &other)
            object_ = take(other.object_);
        return *this;
    }
    static WebGLClipping fromObject(const JSValue& value) { return {value, Existing{}}; }
    JSValue object() const { return copy(object_); }
    operator JSValue() const { return object(); }
    // Wrapper and JSMember views remain confined to the owning runtime thread.
    JSMember uniform{&object_, "uniform"}, numPlanes{&object_, "numPlanes"},
        numIntersection{&object_, "numIntersection"};
    JSValue init(const JSValue& planes = {}, const JSValue& enableLocalClipping = {});
    JSValue beginShadows();
    JSValue endShadows();
    JSValue setGlobalState(const JSValue& planes = {}, const JSValue& camera = {});
    JSValue setState(const JSValue& material = {}, const JSValue& camera = {},
                     const JSValue& useCache = {});
};
