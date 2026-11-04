#pragma once

#include "core/JSValue.h"

#include <string>

// Native counterpart of src/nodes/accessors/Lights.js.
JSValue nativeLightsModule();
JSValue nativeLightsBinding(const std::string& name);
