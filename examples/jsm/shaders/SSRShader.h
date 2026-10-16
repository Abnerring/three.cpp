#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/SSRShader.js (see upstream.json).
// The module exports the SSRBlurShader, SSRDepthShader, SSRShader definitions.
// `nativeSSRShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace SSRShader {
JSValue nativeSSRShader();
} // namespace SSRShader

inline constexpr JSModuleObject SSRShaderModule(SSRShader::nativeSSRShader);
