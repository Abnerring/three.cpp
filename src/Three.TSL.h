#pragma once

#include "Three.Core.h"

// Aggregator port of src/Three.TSL.js from the pinned three.js revision (see upstream.json).
//
// The pinned file is two lines: it imports the core aggregator and re-exports everything from
// nodes/TSL.js. This header reproduces the import that has a C++ counterpart and documents the
// re-export that does not yet exist:
//
//   import './Three.Core.js';          -> #include "Three.Core.h"
//   export * from './nodes/TSL.js';    -> not ported yet (see docs/port-status.json)
//
// Because the re-export target is still missing, this aggregator is a partial port: the TSL entry
// point cannot offer the TSL API surface until nodes/TSL.js and its dependencies are ported.
