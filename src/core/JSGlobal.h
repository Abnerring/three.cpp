#pragma once
#include "core/JSValue.h"

// Native host capabilities used by transplanted source identifiers. Each heap
// owns its bindings; this does not execute scripts or embed a browser runtime.
// Hosts may install constructor values/accessors (for example ImageBitmap).
JSValue nativeGlobalObject();
JSValue nativeGlobalBinding(const std::string& name);
