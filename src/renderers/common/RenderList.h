#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeRenderListConstructor();
JSValue nativeRenderListModule();
JSValue nativeRenderListBinding(const std::string& name);
