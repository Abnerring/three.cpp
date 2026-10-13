#pragma once
#include "core/JSModuleObject.h"
JSValue nativeShaderChunk();
// Stateless resolver; explicit JSValue handles retain the current realm object.
inline constexpr JSModuleObject ShaderChunk(nativeShaderChunk);
