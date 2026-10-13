#pragma once
#include "core/JSValue.h"

// Frame-scoped geometry, instancing and skeleton update protocol from three.js.
class WebGLObjects {
    JSValue object_;
  public:
    WebGLObjects(JSValue gl = {}, JSValue geometries = {}, JSValue attributes = {}, JSValue bindingStates = {}, JSValue info = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue update(JSValue object = {});
    JSValue dispose();
};
