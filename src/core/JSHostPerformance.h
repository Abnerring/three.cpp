#pragma once
#include "core/JSValue.h"

// Native host clock used by source performance.now(). This adapts the platform
// boundary without embedding a browser or compiling JavaScript source.
JSValue nativeHostPerformanceObject();
