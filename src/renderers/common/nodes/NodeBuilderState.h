#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeNodeBuilderStateConstructor();
JSValue nativeNodeBuilderStateModule();
JSValue nativeNodeBuilderStateBinding(const std::string& name);
