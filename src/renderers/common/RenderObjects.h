#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeRenderObjectsConstructor();
JSValue nativeRenderObjectsModule();
JSValue nativeRenderObjectsBinding(const std::string& name);
