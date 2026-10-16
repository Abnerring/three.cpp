#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/ConvolutionShader.js (see upstream.json).
// The module exports the ConvolutionShader definition.
// `nativeConvolutionShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace ConvolutionShader {
JSValue nativeConvolutionShader();
} // namespace ConvolutionShader

inline constexpr JSModuleObject ConvolutionShaderModule(ConvolutionShader::nativeConvolutionShader);
