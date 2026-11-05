#pragma once
#include "core/JSValue.h"
#include <string>
JSValue nativeMaterialXNodesModule();
JSValue nativeMaterialXNodesBinding(const std::string& name);
JSValue nativeMaterialXNodesPrivate(const std::string& name);
