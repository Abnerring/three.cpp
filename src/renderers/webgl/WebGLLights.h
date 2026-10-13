#pragma once
#include "core/JSMember.h"
#include <utility>

// Pinned WebGLLights CPU uniform/cache protocol. The extension object supplies
// has(); light, shadow, texture and matrix inputs use their managed protocols.
// This module does not submit GPU lighting or allocate shadow maps.
class WebGLLights {
    JSValue object_;
    struct Existing {};
    WebGLLights(JSValue object, Existing) : object_(std::move(object)) {}
  public:
    explicit WebGLLights(JSValue extensions = {});
    WebGLLights(const WebGLLights& other) : object_(other.object_) {}
    WebGLLights(WebGLLights&& other) noexcept : object_(std::move(other.object_)) {}
    WebGLLights& operator=(const WebGLLights& other) { object_=other.object_; return *this; }
    WebGLLights& operator=(WebGLLights&& other) noexcept { object_=std::move(other.object_); return *this; }
    static WebGLLights fromObject(JSValue object) { return {std::move(object), Existing{}}; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSMember state{&object_,"state"};
    JSValue setup(JSValue lights = {});
    JSValue setupView(JSValue lights = {}, JSValue camera = {});
};
