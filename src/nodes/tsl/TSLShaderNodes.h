#pragma once
#include "core/JSValue.h"

// Installs the native ShaderNode/Fn and stack exports after TSLCore's lexical
// helpers are available. Captured module handles are managed graph edges.
void installNativeTSLShaderExports(const JSValue& module);
