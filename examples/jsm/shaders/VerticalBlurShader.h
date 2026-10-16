#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/VerticalBlurShader.js (see upstream.json).
// The module exports the VerticalBlurShader definition.
// `nativeVerticalBlurShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace VerticalBlurShader {
JSValue nativeVerticalBlurShader();
} // namespace VerticalBlurShader

inline constexpr JSModuleObject VerticalBlurShaderModule(VerticalBlurShader::nativeVerticalBlurShader);
