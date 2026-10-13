#pragma once
#include "core/JSModuleObject.h"
JSValue nativeShaderLib();
// Stateless resolver; explicit JSValue handles retain the current realm object.
inline constexpr JSModuleObject ShaderLib(nativeShaderLib);
