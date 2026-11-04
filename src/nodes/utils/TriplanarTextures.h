#pragma once

#include "core/JSModuleObject.h"

// Native counterpart of src/nodes/utils/TriplanarTextures.js.
JSValue nativeTriplanarTexturesModule();
JSValue nativeTriplanarTexturesBinding(const std::string& name);
JSValue nativeTriplanarTextures();
JSValue nativeTriplanarTexture();
