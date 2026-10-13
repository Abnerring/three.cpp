#pragma once
#include "core/JSValue.h"

// The upstream CPU geometry protocol; GL commands and binding state operations
// are provided by managed native interface objects. No GPU context is created.
class WebGLGeometries {
    JSValue object_;
  public:
    WebGLGeometries(JSValue gl = {}, JSValue attributes = {}, JSValue info = {}, JSValue bindingStates = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue get(JSValue object = {}, JSValue geometry = {});
    JSValue update(JSValue geometry = {});
    JSValue getWireframeAttribute(JSValue geometry = {});
    JSValue dispose();
};
