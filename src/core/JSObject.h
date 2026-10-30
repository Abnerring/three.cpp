#pragma once
#include "core/JSValue.h"
// Installs Object.assign/values/keys/getPrototypeOf/defineProperty on the realm intrinsic.
// String/Number/Boolean boxing uses real wrapper intrinsics; Symbol boxing remains unsupported.
void initializeNativeObjectMethods(const JSValue& constructor);
