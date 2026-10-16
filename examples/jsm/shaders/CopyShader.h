#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/CopyShader.js (see upstream.json).
// The module exports the CopyShader definition.
// `nativeCopyShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace CopyShader {
JSValue nativeCopyShader();
} // namespace CopyShader

inline constexpr JSModuleObject CopyShaderModule(CopyShader::nativeCopyShader);
