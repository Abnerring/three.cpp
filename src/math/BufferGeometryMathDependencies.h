#pragma once
#include "core/JSValue.h"
// Call only during the actual math constructor's prototype initialization.
// Geometry import must not overwrite a prototype already changed by a user.
void installBufferGeometryMatrix4Methods(const JSValue& prototype);
void installBufferGeometryVector2Methods(const JSValue& prototype);
