#pragma once
#include "core/JSModuleObject.h"
#include "core/JSValue.h"

// Native port of the pinned examples/jsm/shaders/FilmShader.js (see upstream.json).
// The module exports the FilmShader definition.
// `nativeFilmShader()` returns the module namespace object whose keys are the
// upstream export names in module namespace order.
namespace FilmShader {
JSValue nativeFilmShader();
} // namespace FilmShader

inline constexpr JSModuleObject FilmShaderModule(FilmShader::nativeFilmShader);
