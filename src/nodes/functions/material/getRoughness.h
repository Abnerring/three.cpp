#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeGetRoughness();
JSValue nativeGetRoughnessModule();
JSValue nativeGetRoughnessBinding(const std::string& name);
