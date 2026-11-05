#pragma once

#include "scenes/Scene.h"

// Native port of examples/jsm/environments/DebugEnvironment.js from the pinned three.js revision
// (see upstream.json).
//
// A small demonstration environment: a room box with a standard material facing inwards, one point
// light and three emissive boxes standing in for coloured lights. dispose collects the meshes through
// traverse and releases every unique geometry and material exactly once, which matters because the four
// meshes share one box geometry.
class DebugEnvironment : public Scene {
  public:
    DebugEnvironment();
    void dispose() override;
};
