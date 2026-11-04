#pragma once

#include "core/JSValue.h"
#include <string>

// Native module for src/nodes/functions/BSDF/BRDF_Lambert.js at the pinned three.js revision.
JSValue nativeBRDFLambert();
JSValue nativeBRDFLambertModule();
JSValue nativeBRDFLambertBinding(const std::string& name);

