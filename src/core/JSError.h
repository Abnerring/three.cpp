#pragma once
#include "core/JSValue.h"

JSValue nativeErrorConstructor();
bool isNativeError(const JSValue& value);
// An explicit provider returns host call sites. Undefined captures the real
// translated-source execution frames. Formatting remains lazy and consults the
// current Error.prepareStackTrace.
void setNativeErrorStackCapture(const JSValue& callback);

// Realm-private NativeError intrinsics. Global bindings and prototype.constructor
// are mutable aliases and are never used to select these constructors.
JSValue nativeEvalErrorConstructor();
JSValue nativeRangeErrorConstructor();
JSValue nativeReferenceErrorConstructor();
JSValue nativeSyntaxErrorConstructor();
JSValue nativeTypeErrorConstructor();
JSValue nativeURIErrorConstructor();
