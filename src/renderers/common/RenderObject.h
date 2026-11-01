#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeRenderObjectConstructor();
JSValue nativeRenderObjectModule();
JSValue nativeRenderObjectBinding(const std::string& name);
