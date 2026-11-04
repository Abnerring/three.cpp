#pragma once

#include "core/JSValue.h"
#include <string>

// Native module for src/nodes/functions/BSDF/Schlick_to_F0.js at the pinned revision.
JSValue nativeSchlickToF0();
JSValue nativeSchlickToF0Module();
JSValue nativeSchlickToF0Binding(const std::string& name);
