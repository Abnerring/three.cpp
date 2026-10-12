#pragma once
#include "core/JSMember.h"
#include <utility>

// Plain managed objects from the pinned WebGL render-list factories. This CPU
// ordering/cache layer emits no GPU calls and keeps native wrapper identities.
class WebGLRenderList {
    JSValue object_;
    struct Existing {};
    WebGLRenderList(JSValue object, Existing) : object_(std::move(object)) {}
  public:
    WebGLRenderList();
    WebGLRenderList(const WebGLRenderList& other) : object_(other.object_) {}
    WebGLRenderList(WebGLRenderList&& other) noexcept : object_(std::move(other.object_)) {}
    WebGLRenderList& operator=(const WebGLRenderList& other) { object_=other.object_; return *this; }
    WebGLRenderList& operator=(WebGLRenderList&& other) noexcept { object_=std::move(other.object_); return *this; }
    static WebGLRenderList fromObject(JSValue object) { return {std::move(object), Existing{}}; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSMember opaque{&object_,"opaque"}, transmissive{&object_,"transmissive"}, transparent{&object_,"transparent"};
    JSValue init();
    JSValue push(JSValue object={},JSValue geometry={},JSValue material={},JSValue groupOrder={},JSValue z={},JSValue group={},JSValue camera={});
    JSValue unshift(JSValue object={},JSValue geometry={},JSValue material={},JSValue groupOrder={},JSValue z={},JSValue group={});
    JSValue sort(JSValue customOpaqueSort={},JSValue customTransparentSort={});
    JSValue finish();
};
class WebGLRenderLists {
    JSValue object_;
  public:
    WebGLRenderLists();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue get(JSValue scene={},JSValue renderCallDepth={});
    JSValue dispose();
};
