#pragma once
#include "math/Vector3.h"

// Native port of the pinned examples/jsm/geometries/ParametricFunctions.js.
//
// The parameter names and their order are kept exactly as upstream, including klein(),
// whose first parameter is named `v` and whose second is named `u`, so a caller that
// passes ( u, v, target ) like the other functions gets the upstream swap.
namespace ParametricFunctions {
void klein(double v, double u, Vector3& target);
void plane(double u, double v, Vector3& target);
void mobius(double u, double t, Vector3& target);
void mobius3d(double u, double t, Vector3& target);
}
