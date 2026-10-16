#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/SSAOShader.js (see upstream.json).
// The module exports the SSAOBlurShader, SSAODepthShader, SSAOShader definitions.
// `nativeSSAOShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace SSAOShader {
JSValue nativeSSAOShader();
} // namespace SSAOShader

inline constexpr JSModuleObject SSAOShaderModule(SSAOShader::nativeSSAOShader);
