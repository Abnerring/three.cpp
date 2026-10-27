#pragma once
#include "core/JSValue.h"

// Canonical TSL module: live node conversion, method chaining, swizzles,
// callable shader graphs and native function constructor proxies.
JSValue nativeTSLCore();
// Internal imports use the original heap-owned bindings, independently of the
// writable compatibility facade returned by nativeTSLCore().
JSValue nativeTSLCoreBinding(const std::string& name);
