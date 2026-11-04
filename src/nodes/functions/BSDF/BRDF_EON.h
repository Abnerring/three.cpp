#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeBRDFEON();
JSValue nativeBRDFEONModule();
JSValue nativeBRDFEONBinding(const std::string& name);
