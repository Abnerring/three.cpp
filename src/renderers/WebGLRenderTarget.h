#pragma once
#include "core/JSValue.h"

// Shared canonical module. All source renderer instances import this same
// constructor/prototype; inherited RenderTarget methods remain source methods.
JSValue nativeWebGLRenderTargetModule();
JSValue nativeWebGLRenderTargetBinding(const std::string& name);
JSValue nativeWebGLRenderTargetConstructor();

class WebGLRenderTarget {
    JSValue object_;
  public:
    explicit WebGLRenderTarget(JSValue width = 1, JSValue height = 1,
                              JSValue options = JSValue::object())
        : object_(nativeWebGLRenderTargetConstructor().construct({width, height, options})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
};
