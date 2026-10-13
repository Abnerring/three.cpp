#pragma once
#include "core/JSValue.h"
JSValue nativeWebGLShaderConstructor();
// Injected CPU shader creation/compilation protocol; GPU compilation is separate.
JSValue WebGLShader(const JSValue& gl = {}, const JSValue& type = {}, const JSValue& source = {});
