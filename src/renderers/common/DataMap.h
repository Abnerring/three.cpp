#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeDataMapConstructor();
JSValue nativeDataMapModule();
JSValue nativeDataMapBinding(const std::string& name);
