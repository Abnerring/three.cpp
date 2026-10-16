#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/PoissonDenoiseShader.js (see upstream.json).
// The module exports PoissonDenoiseShader and generatePdSamplePointInitializer.
// `nativePoissonDenoiseShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace PoissonDenoiseShader {
JSValue nativePoissonDenoiseShader();
} // namespace PoissonDenoiseShader

inline constexpr JSModuleObject PoissonDenoiseShaderModule(PoissonDenoiseShader::nativePoissonDenoiseShader);
