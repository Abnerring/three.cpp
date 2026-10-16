#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/GammaCorrectionShader.js (see upstream.json).
// The module exports the GammaCorrectionShader definition.
// `nativeGammaCorrectionShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace GammaCorrectionShader {
JSValue nativeGammaCorrectionShader();
} // namespace GammaCorrectionShader

inline constexpr JSModuleObject GammaCorrectionShaderModule(GammaCorrectionShader::nativeGammaCorrectionShader);
