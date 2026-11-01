#pragma once
#include "core/JSValue.h"
#include <string>
// Native compatibility facade; private bindings are distinct from its mutable fields.
JSValue nativeQuadMeshModule();
JSValue nativeQuadMeshBinding(const std::string& name);
JSValue nativeQuadMeshConstructor();
