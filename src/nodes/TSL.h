#pragma once

// Aggregator port of src/nodes/TSL.js from the pinned three.js revision (see upstream.json).
//
// The pinned file is a very large re-export list (constants, core nodes, math nodes, accessors,
// display, lighting, utils, ...). Only the TSL foundation modules exist in C++ today, so this header
// includes those and leaves the rest of the aggregation to the modules that still have to be ported:
//
//   available now:
//     nodes/tsl/TSLBase.h            (float, vec2..vec4, Fn and the arithmetic helpers)
//     nodes/tsl/TSLCore.h            (core TSL entry points)
//     nodes/tsl/TSLShaderNodes.h     (shader node helpers)
//     nodes/tsl/TSLFunctionRuntime.h (function node runtime)
//
//   not aggregated yet (their modules are still missing):
//     ./core/constants.js and the ./core/*Node.js family (Assign, Attribute, Bypass, Isolate,
//     Context, OverrideContext, Index, Parameter, Property, Stack, Struct, UniformGroup, Uniform,
//     Varying, OutputStruct, MRT)
//     ./math/* (Bitcast, Bitcount, Hash, ...)
//     ./accessors/*, ./display/*, ./lighting/*, ./utils/* and the remaining export groups
//
// This is an honest partial aggregator: it exposes the TSL objects that exist and does not pretend
// to re-export the ones that do not.
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/tsl/TSLShaderNodes.h"
