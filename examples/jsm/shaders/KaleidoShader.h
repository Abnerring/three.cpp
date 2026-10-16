#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/KaleidoShader.js (see upstream.json).
// The module exports the KaleidoShader definition.
// `nativeKaleidoShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace KaleidoShader {
JSValue nativeKaleidoShader();
} // namespace KaleidoShader

inline constexpr JSModuleObject KaleidoShaderModule(KaleidoShader::nativeKaleidoShader);
