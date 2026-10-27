#pragma once
#include "core/JSValue.h"
// Installs Object.assign/values/keys/getPrototypeOf/defineProperty on the realm intrinsic.
// getPrototypeOf uses existing String/Number boxing. Object.assign currently
// boxes only String targets; Boolean/Symbol boxing remains unsupported.
void initializeNativeObjectMethods(const JSValue& constructor);
