#pragma once
#include "core/JSValue.h"

JSValue getFloatLength(const JSValue& length);
JSValue getStrideLength(const JSValue& length);
JSValue getVectorLength(const JSValue& count, const JSValue& vectorLength = {});
JSValue nativeBufferUtils();
