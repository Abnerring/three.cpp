#pragma once

// Aggregator port of src/nodes/Nodes.js from the pinned three.js revision (see upstream.json).
//
// The pinned file re-exports the node class families used by the node based renderer: the core node
// types, accessors, lighting nodes, functions and display nodes. Only a subset of those families has
// a C++ counterpart today, so this header includes what exists and keeps the rest visible instead of
// pretending to aggregate a complete node surface:
//
//   available now:
//     nodes/core/PropertyNode.h        (property and varying property nodes)
//     nodes/accessors/MaterialNode.h   (material property access nodes)
//     nodes/loading/BasicEnvironmentNode.h -> nodes/lighting/BasicEnvironmentNode.h
//     nodes/lighting/LightingNodeRuntime.h
//     nodes/functions/PhongLightingModel.h
//     nodes/tsl/TSLBase.h
//
//   not aggregated yet (their modules are still missing):
//     ./core/* remaining node types, ./accessors/* beyond MaterialNode, ./display/*,
//     ./lighting/* beyond the lighting model runtime, ./function/* beyond PhongLightingModel,
//     ./materialx/*, ./utils/* and the renderer side node graph entry points
//
// Because the majority of the export groups is still unported, this is an honest partial aggregator.
#include "nodes/accessors/MaterialNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/functions/PhongLightingModel.h"
#include "nodes/lighting/BasicEnvironmentNode.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLBase.h"
