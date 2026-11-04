#pragma once

#include "core/JSModuleObject.h"

// Native counterpart of src/nodes/procedural/Checker.js.
JSValue nativeCheckerModule();
JSValue nativeCheckerBinding(const std::string& name);
JSValue nativeChecker();
