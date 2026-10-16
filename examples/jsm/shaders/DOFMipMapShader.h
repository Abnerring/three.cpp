#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/DOFMipMapShader.js (see upstream.json).
// The module exports the DOFMipMapShader definition.
// `nativeDOFMipMapShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace DOFMipMapShader {
JSValue nativeDOFMipMapShader();
} // namespace DOFMipMapShader

inline constexpr JSModuleObject DOFMipMapShaderModule(DOFMipMapShader::nativeDOFMipMapShader);
