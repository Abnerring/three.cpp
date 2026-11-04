#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeVelocityNodeConstructor();
JSValue nativeVelocityNodeModule();
JSValue nativeVelocityNodeBinding(const std::string& name);
