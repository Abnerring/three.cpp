#pragma once

#include "core/JSValue.h"

// Native counterpart of src/nodes/utils/UVUtils.js.
JSValue nativeUVUtilsModule();
JSValue nativeUVUtilsBinding(const std::string& name);
JSValue nativeReplaceDefaultUV();
JSValue nativeRotateUV();
JSValue nativeSpherizeUV();
