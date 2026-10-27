#pragma once
#include "core/JSValue.h"

JSValue nativeErrorConstructor();
bool isNativeError(const JSValue& value);
// Capture returns an array of native host call sites. Undefined uses an empty
// list. Formatting is lazy and consults the current Error.prepareStackTrace.
void setNativeErrorStackCapture(const JSValue& callback);
