#pragma once

#include "core/JSValue.h"

// Port of src/Three.Legacy.js at the pinned revision.
//
// The upstream file is empty (zero bytes): every legacy alias it used to carry has been removed from the
// public API, so the faithful port is a module with no exports at all.
JSValue nativeThreeLegacyModule();
