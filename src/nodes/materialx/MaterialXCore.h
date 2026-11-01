#pragma once
#include "core/JSValue.h"
#include <string>
// Real lexical exports; mutable compatibility facade does not replace lexical bindings.
JSValue nativeMaterialXCoreModule();
JSValue nativeMaterialXCoreBinding(const std::string& name);
