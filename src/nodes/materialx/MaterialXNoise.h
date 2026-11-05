#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeMaterialXNoiseModule();
JSValue nativeMaterialXNoiseBinding(const std::string& name);
// Original lexical functions and imports for source/graph validation.
JSValue nativeMaterialXNoisePrivate(const std::string& name);
