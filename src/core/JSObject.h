#pragma once
#include "core/JSValue.h"
// Installs Object.assign/values/keys/getPrototypeOf/defineProperty on the realm intrinsic.
// Primitive targets use the real String/Number/Boolean/Symbol wrapper intrinsics.
void initializeNativeObjectMethods(const JSValue& constructor);
