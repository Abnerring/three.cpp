#pragma once
#include "core/JSValue.h"
// Lexical identities shared by the native curve modules. This private class
// table contains Curve/CurvePath/Path/Shape as well as the ten Curves exports.
JSValue nativeCurveClassTable();
