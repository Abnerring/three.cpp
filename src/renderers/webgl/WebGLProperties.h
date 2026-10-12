#pragma once
#include "core/JSValue.h"

// Managed handle for the plain closure object returned by WebGLProperties.
class WebGLProperties {
    JSValue object_;
  public:
    WebGLProperties();
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue has(JSValue object = {});
    JSValue get(JSValue object = {});
    JSValue remove(JSValue object = {});
    JSValue update(JSValue object = {}, JSValue key = {}, JSValue value = {});
    JSValue dispose();
};
