#pragma once
#include "core/BufferGeometry.h"

// Native port of the pinned examples/jsm/geometries/BoxLineGeometry.js.
//
// The upstream class sets only the position attribute, so this port also leaves `type`
// and `parameters` untouched, like the pinned source does.
class BoxLineGeometry : public BufferGeometry {
  public:
    BoxLineGeometry(double width = 1, double height = 1, double depth = 1, double widthSegments = 1,
                    double heightSegments = 1, double depthSegments = 1);
};
