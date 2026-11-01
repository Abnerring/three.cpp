#pragma once
#include "core/JSValue.h"
#include <string>
// Private lexical exports, independent of mutable native compatibility facades.
JSValue nativeShaderMathUtilsModule();
JSValue nativeShaderMathUtilsBinding(const std::string& name);
