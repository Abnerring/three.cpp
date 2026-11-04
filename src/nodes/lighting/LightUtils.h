#pragma once

#include "core/JSValue.h"

#include <string>

// Native module for src/nodes/lighting/LightUtils.js at the pinned revision.
JSValue nativeLightUtilsModule();
JSValue nativeLightUtilsBinding(const std::string& name);
JSValue nativeGetDistanceAttenuation();
