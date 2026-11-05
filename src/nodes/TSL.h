#pragma once

// Partial native header aggregation of the pinned nodes/TSL.js export surface.
// It includes the TSL foundation and MaterialX public module. Other native node
// modules can be included directly; their presence does not imply that this
// aggregate or the renderer execution path is complete. See docs/port-status.json.
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/tsl/TSLFunctionRuntime.h"
#include "nodes/tsl/TSLShaderNodes.h"

#include "nodes/materialx/MaterialXNodes.h"
