#pragma once
#include "core/JSValue.h"

JSValue nativeWebGLMorphtargetsConstructor();
// Original morph packing/cache/update/dispose logic over native GPU services.
class WebGLMorphtargets {
    JSValue object_;
  public:
    explicit WebGLMorphtargets(JSValue gl = {}, JSValue capabilities = {}, JSValue textures = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
};
