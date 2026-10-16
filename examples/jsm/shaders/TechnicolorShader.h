#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/TechnicolorShader.js (see upstream.json).
// The module exports the TechnicolorShader definition.
// `nativeTechnicolorShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace TechnicolorShader {
JSValue nativeTechnicolorShader();
} // namespace TechnicolorShader

inline constexpr JSModuleObject TechnicolorShaderModule(TechnicolorShader::nativeTechnicolorShader);
