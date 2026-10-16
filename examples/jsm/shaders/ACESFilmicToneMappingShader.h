#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/ACESFilmicToneMappingShader.js (see upstream.json).
// The module exports the ACESFilmicToneMappingShader definition.
// `nativeACESFilmicToneMappingShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace ACESFilmicToneMappingShader {
JSValue nativeACESFilmicToneMappingShader();
} // namespace ACESFilmicToneMappingShader

inline constexpr JSModuleObject ACESFilmicToneMappingShaderModule(ACESFilmicToneMappingShader::nativeACESFilmicToneMappingShader);
