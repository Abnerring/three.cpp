#pragma once
#include "core/JSValue.h"

// Upstream WebGLAttributes CPU protocol. gl is an injected native command
// object; this module neither creates an OpenGL context nor executes JS.
// Typed-array instanceof uses the nine managed native constructors. Native
// Float16Array and Symbol.hasInstance require runtime support not present here;
// Float16BufferAttribute's Uint16Array storage follows the upstream flag path.
class WebGLAttributes {
    JSValue object_;
  public:
    explicit WebGLAttributes(JSValue gl = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSValue get(JSValue attribute = {});
    JSValue remove(JSValue attribute = {});
    JSValue update(JSValue attribute = {}, JSValue bufferType = {});
};
