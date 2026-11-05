#pragma once
#include "core/JSValue.h"
#include "extras/core/Shape.h"
#include "extras/core/Curve.h"
JSValue nativeObjectLoaderShapeConstructor();
JSValue nativeObjectLoaderCurveConstructors();
// Numerical adapters read the current managed curve, never its original JSON input.
std::shared_ptr<Shape> objectLoaderShape(const JSValue& value);
std::shared_ptr<Curve<Vector3>> objectLoaderCurve3(const JSValue& value);
