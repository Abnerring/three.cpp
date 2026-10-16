#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/SAOShader.js (see upstream.json).
// The module exports the SAOShader definition.
// `nativeSAOShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace SAOShader {
JSValue nativeSAOShader();
} // namespace SAOShader

inline constexpr JSModuleObject SAOShaderModule(SAOShader::nativeSAOShader);
