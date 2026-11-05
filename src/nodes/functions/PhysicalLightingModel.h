#pragma once

#include "core/JSValue.h"

#include <string>

// Port of src/nodes/functions/PhysicalLightingModel.js at the pinned revision (see upstream.json).
//
// Stage A lands the class shape: the module and the constructor with the nine feature switches and the
// sixteen cached nodes the model fills in while it runs. The lighting hooks and the module level
// transmission, iridescence and sheen graphs land in later stages.
JSValue nativePhysicalLightingModelModule();
JSValue nativePhysicalLightingModelBinding(const std::string& name);
JSValue nativePhysicalLightingModelConstructor();
// Port only test hooks: build one module level helper graph so its construction can be covered.
JSValue nativePhysicalLightingModelHelper(const std::string& name);