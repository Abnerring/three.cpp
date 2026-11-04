#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeBRDFSheen();
JSValue nativeBRDFSheenModule();
JSValue nativeBRDFSheenBinding(const std::string& name);
