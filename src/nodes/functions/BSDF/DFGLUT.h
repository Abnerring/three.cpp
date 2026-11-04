#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeDFGLUT();
JSValue nativeDFGLUTModule();
JSValue nativeDFGLUTBinding(const std::string& name);
