#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/LuminosityHighPassShader.js (see upstream.json).
// The module exports the LuminosityHighPassShader definition.
// `nativeLuminosityHighPassShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace LuminosityHighPassShader {
JSValue nativeLuminosityHighPassShader();
} // namespace LuminosityHighPassShader

inline constexpr JSModuleObject LuminosityHighPassShaderModule(LuminosityHighPassShader::nativeLuminosityHighPassShader);
