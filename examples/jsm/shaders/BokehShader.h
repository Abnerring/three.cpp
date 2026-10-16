#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/BokehShader.js (see upstream.json).
// The module exports the BokehShader definition.
// `nativeBokehShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace BokehShader {
JSValue nativeBokehShader();
} // namespace BokehShader

inline constexpr JSModuleObject BokehShaderModule(BokehShader::nativeBokehShader);
