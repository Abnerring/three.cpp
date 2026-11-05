#pragma once

#include "Three.Core.h"
#include "materials/nodes/NodeMaterials.h"

// Aggregator port of src/Three.WebGPU.js from the pinned three.js revision (see upstream.json).
//
// The pinned file re-exports the core surface, the node materials aggregator and the WebGPU renderer
// stack. As with Three.WebGPU.Nodes.js only the core aggregator has a C++ counterpart today; the
// re-export list is reproduced here so the missing pieces stay visible:
//
//   export * from './Three.Core.js';                        -> #include "Three.Core.h"
//   export * from './materials/nodes/NodeMaterials.js';     -> NodeMaterials.h
//   WebGPURenderer           -> not ported yet
//   WebGPUBackend            -> not ported yet
//   WebGLBackend             -> not ported yet
//   Renderer                 -> not ported yet
//   Backend                  -> not ported yet
//   WebGLCapabilities        -> not ported yet
//   Lighting                 -> not ported yet
//   BundleGroup              -> not ported yet
//   QuadMesh                 -> not ported yet
//
// The renderer stack in this port targets Vulkan, so this aggregator stays partial until those
// modules land; it must not pretend to offer them earlier.
