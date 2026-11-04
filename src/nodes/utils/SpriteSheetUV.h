#pragma once

#include "core/JSModuleObject.h"

// Port of pinned src/nodes/utils/SpriteSheetUV.js. The exported Fn computes the UV of the
// selected frame in a sprite sheet, preserving the source's row order and defaults.
JSValue nativeSpriteSheetUVModule();
JSValue nativeSpriteSheetUVBinding(const std::string& name);
JSValue nativeSpriteSheetUV();
