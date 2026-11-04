#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeGetGeometryRoughness();
JSValue nativeGetGeometryRoughnessModule();
JSValue nativeGetGeometryRoughnessBinding(const std::string& name);
