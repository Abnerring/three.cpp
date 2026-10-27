#pragma once
#include "core/JSModuleObject.h"
JSValue nativeUVModule();
JSValue nativeUVBinding(const std::string& name);
inline constexpr JSModuleObject UV{nativeUVModule};
