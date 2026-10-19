#pragma once
#include "core/Object3D.h"
#include <memory>

class PerspectiveCamera;

// Port of src/cameras/CubeCamera.js at the pinned revision, the camera rig only.
//
// The portable half is the rig itself: six perspective cameras at a negative ninety degree field of view and
// unit aspect, sharing this object layers, added as children, and re-oriented by updateCoordinateSystem to match
// the handedness of the renderer it will draw with. The half that is not portable is update and clear, which
// read and restore render target state on a renderer; those refuse by name here rather than pretending.
class CubeCamera : public Object3D {
  public:
    CubeCamera(double near = 0.1, double far = 2000, JSValue renderTarget = {});

    CubeCamera& updateCoordinateSystem();
    // Both of these need a renderer to bind targets on.
    CubeCamera& update(JSValue renderer = {}, JSValue scene = {});
    CubeCamera& clear(JSValue renderer = {});

    JSValue renderTarget, coordinateSystem;
    double activeMipmapLevel = 0;
};