#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/SubsurfaceScatteringShader.js (see upstream.json).
// The module exports the SubsurfaceScatteringShader definition.
// `nativeSubsurfaceScatteringShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace SubsurfaceScatteringShader {
JSValue nativeSubsurfaceScatteringShader();
} // namespace SubsurfaceScatteringShader

inline constexpr JSModuleObject SubsurfaceScatteringShaderModule(SubsurfaceScatteringShader::nativeSubsurfaceScatteringShader);
