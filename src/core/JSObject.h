#pragma once
#include "core/JSValue.h"
// Installs Object.assign/values/keys on the actual realm intrinsic. Number/Boolean/Symbol
// target boxing is not available; those targets raise a native TypeError.
void initializeNativeObjectMethods(const JSValue& constructor);
