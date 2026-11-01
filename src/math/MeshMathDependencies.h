#pragma once
#include "core/JSValue.h"
// Install during the genuine Vector4 constructor prototype initialization, not Mesh import.
void installMeshVector4Methods(const JSValue& prototype);

// Install at the genuine Vector2 constructor, before the prototype is exposed.
void installMeshVector2Methods(const JSValue& prototype);
