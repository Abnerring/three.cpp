#pragma once

#include "objects/Mesh.h"

#include <memory>

class Texture;

// Native port of examples/jsm/objects/GroundedSkybox.js from the pinned three.js revision
// (see upstream.json).
//
// A Mesh around an inverted sphere whose lower half is flattened onto a fixed ground plane: the
// pinned constructor validates its arguments, builds the sphere, mirrors it on z and then rewrites
// every vertex below the equator so the sphere stands on y = -height. The texture is the sky map and
// depth writes are disabled so the sphere never occludes the scene.
class GroundedSkybox : public Mesh {
  public:
    GroundedSkybox(std::shared_ptr<Texture> map, double height, double radius,
                   double resolution = 128);
};
