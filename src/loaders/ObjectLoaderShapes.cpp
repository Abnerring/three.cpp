#include "loaders/ObjectLoaderShapes.h"
#include "extras/core/Shape.h"
#include "extras/curves/Curves.h"

// Compatibility names retain the canonical module and constructor identities.
JSValue nativeObjectLoaderCurveConstructors() { return nativeCurvesModule(); }
JSValue nativeObjectLoaderShapeConstructor() { return nativeShapeConstructor(); }
