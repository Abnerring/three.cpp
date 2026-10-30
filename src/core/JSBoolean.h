#pragma once
#include "core/JSValue.h"

// ECMAScript Boolean intrinsic and actual [[BooleanData]] boxed objects.
JSValue nativeBooleanConstructor();
JSValue nativeBooleanPrototype();
JSValue createNativeBooleanObject(const JSValue& primitiveBoolean);
bool isNativeBooleanObject(const JSValue& value);
JSValue nativeBooleanValue(const JSValue& receiver);
