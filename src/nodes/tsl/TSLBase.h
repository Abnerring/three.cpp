#pragma once
#include "core/JSModuleObject.h"

JSValue nativeTSLBase();
JSValue nativeTSLBaseBinding(const std::string& name);
inline constexpr JSModuleObject TSLBase{nativeTSLBase};
