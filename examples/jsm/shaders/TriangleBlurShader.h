#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/TriangleBlurShader.js (see upstream.json).
// The module exports the TriangleBlurShader definition.
// `nativeTriangleBlurShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace TriangleBlurShader {
JSValue nativeTriangleBlurShader();
} // namespace TriangleBlurShader

inline constexpr JSModuleObject TriangleBlurShaderModule(TriangleBlurShader::nativeTriangleBlurShader);
