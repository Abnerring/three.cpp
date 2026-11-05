#pragma once

#include "Three.Core.h"
#include "materials/nodes/NodeMaterials.h"

// Aggregator port of src/Three.WebGPU.Nodes.js from the pinned three.js revision (see upstream.json).
//
// The pinned file re-exports the core surface, the node materials aggregator and a set of renderer
// classes. Only the aggregators that are ported can be included here; the rest are listed so the gap
// is explicit instead of silently missing:
//
//   export * from './Three.Core.js';                        -> #include "Three.Core.h"
//   export * from './materials/nodes/NodeMaterials.js';     -> NodeMaterials.h (Line2 remains pending)
//   WebGPURenderer           -> not ported yet (renderer side is Vulkan here)
//   WebGPUBackend            -> not ported yet
//   WebGLBackend             -> not ported yet
//   Lighting                 -> not ported yet
//   BundleGroup              -> not ported yet
//   QuadMesh                 -> not ported yet
//   PMREMGenerator           -> not ported yet
//   RenderPipeline           -> not ported yet
//   DirectRenderPipeline     -> not ported yet
//
// This is therefore a partial aggregator: it exposes exactly the same core entry as the pinned file
// and records every re-export that is still missing.
