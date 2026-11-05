#pragma once

#include "core/Object3D.h"

// Native port of examples/jsm/misc/Gyroscope.js from the pinned three.js revision
// (see upstream.json).
//
// The addon keeps only the rotation of its local matrix while still inheriting the world
// translation of its parent, which makes an object appear to keep a fixed orientation in space
// no matter how the parent turns. The override below is line for line the pinned one: compose the
// world translation with the local rotation, then propagate the forced update to the children.
class Gyroscope : public Object3D {
  public:
    Gyroscope();
    void updateMatrixWorld(bool force = false) override;
};
