#pragma once
#include "core/JSMember.h"
#include <utility>

class WebGLRenderState {
    JSValue object_;
    struct Existing {};
    WebGLRenderState(JSValue object,Existing):object_(std::move(object)) {}
  public:
    explicit WebGLRenderState(JSValue extensions = {});
    WebGLRenderState(const WebGLRenderState& other):object_(other.object_) {}
    WebGLRenderState(WebGLRenderState&& other) noexcept:object_(std::move(other.object_)) {}
    WebGLRenderState& operator=(const WebGLRenderState& other) {object_=other.object_;return *this;}
    WebGLRenderState& operator=(WebGLRenderState&& other) noexcept {object_=std::move(other.object_);return *this;}
    static WebGLRenderState fromObject(JSValue object) {return {std::move(object),Existing{}};}
    JSValue object() const {return object_;}
    operator JSValue() const {return object_;}
    JSMember state{&object_,"state"};
    JSValue init(JSValue camera={});
    JSValue pushLight(JSValue light={});
    JSValue pushShadow(JSValue light={});
    JSValue setupLights();
    JSValue setupLightsView(JSValue camera={});
};
class WebGLRenderStates {
    JSValue object_;
  public:
    explicit WebGLRenderStates(JSValue extensions = {});
    JSValue object() const {return object_;}
    operator JSValue() const {return object_;}
    JSValue get(JSValue scene={},JSValue renderCallDepth=0);
    JSValue dispose();
};
