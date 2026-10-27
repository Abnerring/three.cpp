#pragma once
#include "core/JSValue.h"

// Native host capabilities used by transplanted source identifiers. Each heap
// owns its bindings; this does not execute scripts or embed a browser runtime.
// Hosts may install constructor values/accessors (for example ImageBitmap).
// The initial realm supplies Math.sin/cos; remaining Math API is not installed.
// Math and its methods retain native writable/configurable host semantics.
// String supplies the callable/constructible conversion and fromCharCode;
// its existing native StringData prototype remains shared. Other String static
// methods (fromCodePoint/raw) are not installed yet.
JSValue nativeGlobalObject();
JSValue nativeGlobalBinding(const std::string& name);
