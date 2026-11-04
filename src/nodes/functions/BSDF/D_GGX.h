#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeDGGX();
JSValue nativeDGGXModule();
JSValue nativeDGGXBinding(const std::string& name);
