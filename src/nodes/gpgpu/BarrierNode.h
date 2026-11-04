#pragma once

#include "core/JSValue.h"

#include <string>

// Native counterpart of src/nodes/gpgpu/BarrierNode.js.
JSValue nativeBarrierNodeModule();
JSValue nativeBarrierNodeBinding(const std::string& name);
JSValue nativeBarrierNodeConstructor();

