#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeRenderListsConstructor();
JSValue nativeRenderListsModule();
JSValue nativeRenderListsBinding(const std::string& name);
