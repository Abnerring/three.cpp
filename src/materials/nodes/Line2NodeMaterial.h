#pragma once
#include "core/JSValue.h"

JSValue nativeLine2NodeMaterialModule();
JSValue nativeLine2NodeMaterialBinding(const std::string& name);
JSValue nativeLine2NodeMaterialConstructor();
// Private module bindings for source parity and inspection of the real TSL graph.
JSValue nativeLine2NodeMaterialHelper(const std::string& name);
