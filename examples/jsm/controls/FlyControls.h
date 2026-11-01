#pragma once

#include "core/JSModuleObject.h"
JSValue nativeFlyControlsModule();
JSValue nativeFlyControlsBinding(const std::string& name);
JSValue nativeFlyControlsConstructor();
inline constexpr JSModuleObject FlyControlsModule{nativeFlyControlsModule};
