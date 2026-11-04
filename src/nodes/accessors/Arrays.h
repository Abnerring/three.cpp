#pragma once

#include "core/JSValue.h"

#include <string>

// Native counterpart of src/nodes/accessors/Arrays.js.
JSValue nativeArraysModule();
JSValue nativeArraysBinding(const std::string& name);

