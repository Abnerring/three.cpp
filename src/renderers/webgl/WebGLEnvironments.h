#pragma once
#include "core/JSValue.h"

// Optional imports are native constructor bindings named PMREMGenerator and
// WebGLCubeRenderTarget. Unported defaults throw only when their source branch
// reaches construction. No substitute render target or PMREM is manufactured.
JSValue nativeWebGLEnvironmentsConstructor(JSValue dependencies = {});
class WebGLEnvironments {
    JSValue object_;
  public:
    explicit WebGLEnvironments(JSValue renderer = {}, JSValue dependencies = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
};
