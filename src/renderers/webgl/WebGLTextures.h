#pragma once
#include "core/JSValue.h"

JSValue nativeWebGLTexturesConstructor();
// The original resource manager. GPU entry points are injected native services.
class WebGLTextures {
    JSValue object_;
  public:
    explicit WebGLTextures(JSValue gl={},JSValue extensions={},JSValue state={},JSValue properties={},JSValue capabilities={},JSValue utils={},JSValue info={});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
};
