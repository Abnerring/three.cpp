#pragma once

#include "scenes/Scene.h"

// Native port of examples/jsm/environments/ColorEnvironment.js from the pinned three.js revision
// (see upstream.json).
//
// The addon is a Scene whose only child is a unit sphere with a basic material, used as a cheap
// uniform environment: the sphere is rendered from the inside and its flat colour becomes the
// lighting environment. The constructor builds the child exactly as the pinned one does, and
// dispose releases the geometry and material of that child.
class ColorEnvironment : public Scene {
  public:
    explicit ColorEnvironment(int color = 0xffffff);
    void dispose() override;
};
