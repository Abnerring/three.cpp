#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeUnpackFloatNodeModule();
JSValue nativeUnpackFloatNodeBinding(const std::string& name);
JSValue nativeUnpackFloatNodeConstructor();
