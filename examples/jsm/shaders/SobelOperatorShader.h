#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/SobelOperatorShader.js (see upstream.json).
// The module exports the SobelOperatorShader definition.
// `nativeSobelOperatorShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace SobelOperatorShader {
JSValue nativeSobelOperatorShader();
} // namespace SobelOperatorShader

inline constexpr JSModuleObject SobelOperatorShaderModule(SobelOperatorShader::nativeSobelOperatorShader);
