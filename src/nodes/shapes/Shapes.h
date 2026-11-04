#pragma once

#include "core/JSModuleObject.h"

// Native counterpart of src/nodes/shapes/Shapes.js.
JSValue nativeShapesModule();
JSValue nativeShapesBinding(const std::string& name);
JSValue nativeShapeCircle();
