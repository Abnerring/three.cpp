#pragma once

#include "core/JSValue.h"

#include <string>

// Native counterpart of src/nodes/accessors/SceneProperties.js.
JSValue nativeScenePropertiesModule();
JSValue nativeScenePropertiesBinding(const std::string& name);

