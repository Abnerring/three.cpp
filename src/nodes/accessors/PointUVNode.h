#pragma once

#include "core/JSValue.h"
#include <string>

// Native module for src/nodes/accessors/PointUVNode.js at the pinned three.js revision.
// The small, stateless shader helper remains exposed by core/PointUVNode.h; these bindings
// provide the managed Node object and the TSL pointUV singleton used by the module graph.
JSValue nativePointUVNodeConstructor();
JSValue nativePointUVNodeModule();
JSValue nativePointUVNodeBinding(const std::string& name);
