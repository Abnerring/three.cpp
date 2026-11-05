#pragma once

#include "core/JSValue.h"

// Port of src/renderers/common/RendererUtils.js at the pinned revision. The module is a set of pure
// state helpers: they save, reset and restore renderer and scene state, which is what the shadow passes
// need around their own render calls.
JSValue nativeRendererUtilsModule();
JSValue nativeRendererUtilsBinding(const std::string& name);
