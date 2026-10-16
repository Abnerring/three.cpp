#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/BokehShader2.js (see upstream.json).
// The module exports the BokehDepthShader, BokehShader definitions.
// `nativeBokehShader2()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace BokehShader2 {
JSValue nativeBokehShader2();
} // namespace BokehShader2

inline constexpr JSModuleObject BokehShader2Module(BokehShader2::nativeBokehShader2);
