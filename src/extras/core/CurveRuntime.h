#pragma once
#include "core/JSValue.h"
// Install the original Curve prototype algorithms on the shared native class.
// Curve constructors and their derived classes retain a single object identity.
void installNativeCurveNumericMethods(JSValue curveConstructor, JSValue curveClassTable);
