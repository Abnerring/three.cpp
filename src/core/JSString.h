#pragma once
#include "core/JSValue.h"

// StringData objects and the string methods needed by native source protocols.
// The complete callable/constructible global String API is not supplied here.
JSValue createNativeStringPrototype();
JSValue createNativeStringObject(const JSValue& primitiveString);
bool isNativeStringObject(const JSValue& value);
JSValue nativeStringValue(const JSValue& receiver);
