#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeBRDFEON();
JSValue nativeEONDirectionalAlbedo();
JSValue nativeBRDFEONModule();
JSValue nativeBRDFEONBinding(const std::string& name);
