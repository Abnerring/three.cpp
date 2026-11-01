#pragma once
#include "core/JSValue.h"
#include <string>
// Private lexical exports, independent of mutable native compatibility facades.
JSValue nativeHashModule();
JSValue nativeHashBinding(const std::string& name);
