#pragma once
#include "core/JSValue.h"

// Canonical live subset of TSLCore.js: nodeObject/getConstNodeType and their
// primitive constant cache. Shader/Fn construction and other TSL exports remain
// separate, unimplemented module work.
JSValue nativeTSLCore();
