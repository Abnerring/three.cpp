#pragma once

#include "core/JSValue.h"

// Port of src/nodes/display/PassNode.js at the pinned revision (see upstream.json).
//
// Stage A lands the two texture node wrappers: PassTextureNode and PassMultipleTextureNode, which hold a
// pass node and sample its output texture. The PassNode class itself (render targets, resolution, layers,
// MRT, setup and the frame update) lands in later stages.
JSValue nativePassNodeModule();
JSValue nativePassNodeBinding(const std::string& name);
JSValue nativePassNodeConstructor();
JSValue nativePassTextureNodeConstructor();
JSValue nativePassMultipleTextureNodeConstructor();