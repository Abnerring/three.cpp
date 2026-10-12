#pragma once
#include "cameras/PerspectiveCamera.h"

class StereoCamera {
  public:
    std::string type = "StereoCamera";
    double aspect = 1, eyeSep = 0.064;
    std::shared_ptr<PerspectiveCamera> cameraL, cameraR;
    struct Cache {
        JSValue focus = nullptr, fov = nullptr, aspect = nullptr, near = nullptr;
        JSValue far = nullptr, zoom = nullptr, eyeSep = nullptr;
    } _cache;
    StereoCamera();
    void update(const PerspectiveCamera& camera);
};
