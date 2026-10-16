#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/GTAOShader.js (see upstream.json).
// The module exports GTAOShader, GTAODepthShader, GTAOBlendShader and generateMagicSquareNoise.
// `nativeGTAOShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace GTAOShader {
JSValue nativeGTAOShader();
} // namespace GTAOShader

inline constexpr JSModuleObject GTAOShaderModule(GTAOShader::nativeGTAOShader);
