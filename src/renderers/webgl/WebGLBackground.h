#pragma once
#include "core/JSValue.h"

JSValue nativeWebGLBackgroundConstructor();

// The original factory receives one environments service with get(texture, usePMREM).
class WebGLBackground {
    JSValue object_;
  public:
    explicit WebGLBackground(JSValue renderer={},JSValue environments={},JSValue state={},JSValue objects={},JSValue alpha={},JSValue premultipliedAlpha={});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue getClearColor() const;
    JSValue getClearAlpha() const;
    JSValue setClearColor(JSValue color={},JSValue alpha={});
    JSValue setClearAlpha(JSValue alpha={});
    JSValue render(JSValue scene={});
    JSValue addToRenderList(JSValue renderList={},JSValue scene={});
    JSValue dispose();
};
