#pragma once

#include "core/JSValue.h"

#include <string>

// Native counterpart of src/nodes/utils/ViewportUtils.js at the pinned three.js revision.
JSValue nativeViewportUtilsModule();
JSValue nativeViewportUtilsBinding(const std::string& name);
JSValue nativeViewportSafeUV();
