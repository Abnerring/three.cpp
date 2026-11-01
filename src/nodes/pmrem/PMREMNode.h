#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativePMREMNodeConstructor();
JSValue nativePMREMNodeModule();
JSValue nativePMREMNodeBinding(const std::string& name);
