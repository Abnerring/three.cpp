#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeChainMapConstructor();
JSValue nativeChainMapModule();
JSValue nativeChainMapBinding(const std::string& name);
