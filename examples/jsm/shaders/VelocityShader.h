#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/VelocityShader.js (see upstream.json).
// The module exports the VelocityShader definition.
// `nativeVelocityShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace VelocityShader {
JSValue nativeVelocityShader();
} // namespace VelocityShader

inline constexpr JSModuleObject VelocityShaderModule(VelocityShader::nativeVelocityShader);
