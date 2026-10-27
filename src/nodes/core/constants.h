#pragma once
#include "core/JSValue.h"
// Mutable objects/arrays behind the pinned module's constant bindings.
JSValue nativeNodeConstants();
JSValue nativeNodeConstantsBinding(const std::string& name);
