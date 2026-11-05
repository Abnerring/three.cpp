#pragma once

#include "core/JSValue.h"

// Port of src/nodes/utils/PostProcessingUtils.js at the pinned revision. Every export is a TSL
// function built with Fn, so each one answers a node when called with nodes.
JSValue nativePostProcessingUtilsModule();
JSValue nativePostProcessingUtilsBinding(const std::string& name);
