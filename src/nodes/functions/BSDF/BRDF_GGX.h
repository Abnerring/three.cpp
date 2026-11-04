#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeBRDFGGX();
JSValue nativeBRDFGGXModule();
JSValue nativeBRDFGGXBinding(const std::string& name);
