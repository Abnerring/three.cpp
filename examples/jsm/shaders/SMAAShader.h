#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/SMAAShader.js (see upstream.json).
// The module exports the SMAABlendShader, SMAAEdgesShader, SMAAWeightsShader definitions.
// `nativeSMAAShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace SMAAShader {
JSValue nativeSMAAShader();
} // namespace SMAAShader

inline constexpr JSModuleObject SMAAShaderModule(SMAAShader::nativeSMAAShader);
