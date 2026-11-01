#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativePMREMGeneratorConstructor();
JSValue nativePMREMGeneratorModule();
JSValue nativePMREMGeneratorBinding(const std::string& name);
