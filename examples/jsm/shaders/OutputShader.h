#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/OutputShader.js (see upstream.json).
// The module exports the OutputShader definition.
// `nativeOutputShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace OutputShader {
JSValue nativeOutputShader();
} // namespace OutputShader

inline constexpr JSModuleObject OutputShaderModule(OutputShader::nativeOutputShader);
