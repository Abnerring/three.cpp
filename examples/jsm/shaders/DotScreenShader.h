#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/DotScreenShader.js (see upstream.json).
// The module exports the DotScreenShader definition.
// `nativeDotScreenShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace DotScreenShader {
JSValue nativeDotScreenShader();
} // namespace DotScreenShader

inline constexpr JSModuleObject DotScreenShaderModule(DotScreenShader::nativeDotScreenShader);
