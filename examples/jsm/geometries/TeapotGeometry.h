#pragma once
#include "core/BufferGeometry.h"

// Native port of the pinned examples/jsm/geometries/TeapotGeometry.js.
//
// Tessellates the Utah teapot database by Martin Newell into triangles. The pinned constructor
// only builds the index (as a Uint32 BufferAttribute) and the position/normal/uv attributes; it
// never assigns `type` or `parameters` and does not override copy()/clone() or toJSON(), so this
// port keeps those BufferGeometry fields and methods at their defaults too.
class TeapotGeometry : public BufferGeometry {
  public:
    /** `size = 50, segments = 10, bottom = true, lid = true, body = true, fitLid = true, blinn = true`. */
    TeapotGeometry(double size = 50, double segments = 10, bool bottom = true, bool lid = true,
                   bool body = true, bool fitLid = true, bool blinn = true);
};
