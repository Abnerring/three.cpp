#pragma once

#include "core/JSValue.h"

// Port of src/nodes/display/ToonOutlinePassNode.js at the pinned revision (see upstream.json).
//
// The node extends the pass node with a back side outline material: it renders the scene normally and adds
// an expanded silhouette draw for toon materials, reusing a per material outline material cache.
JSValue nativeToonOutlinePassNodeModule();
JSValue nativeToonOutlinePassNodeBinding(const std::string& name);
JSValue nativeToonOutlinePassNodeConstructor();