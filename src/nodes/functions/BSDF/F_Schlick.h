#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeFSchlick();
JSValue nativeFSchlickModule();
JSValue nativeFSchlickBinding(const std::string& name);
