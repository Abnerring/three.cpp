#pragma once

#include "core/JSValue.h"

// Port of src/nodes/lighting/ShadowFilterNode.js at the pinned revision. The module has no class: it
// exports three TSL filter functions, each of which answers a node when called with nodes.
JSValue nativeShadowFilterNodeModule();
JSValue nativeShadowFilterNodeBinding(const std::string& name);
