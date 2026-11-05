#pragma once

#include "core/JSValue.h"

#include <string>

// Port of src/nodes/functions/PhysicalLightingModel.js at the pinned revision (see upstream.json).
//
// Native TSL graphs for direct/area/indirect physical lighting, multiscattering,
// sheen, clearcoat, iridescence, rough diffuse reflection and volume transmission.
JSValue nativePhysicalLightingModelModule();
JSValue nativePhysicalLightingModelBinding(const std::string& name);
JSValue nativePhysicalLightingModelConstructor();
// Port-only inspection hook for original lexical imports, constants and helpers.
// The public module continues to export only the default constructor.
JSValue nativePhysicalLightingModelHelper(const std::string& name);
