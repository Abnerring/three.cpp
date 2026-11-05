#pragma once

#include "core/JSValue.h"

// Port of src/nodes/functions/BSDF/LTC.js at the pinned revision. The module exports three TSL
// functions that carry the linearly transformed cosine evaluation for rect area lights.
JSValue nativeLTCModule();
JSValue nativeLTCBinding(const std::string& name);
