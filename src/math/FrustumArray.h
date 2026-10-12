#pragma once
#include "math/Frustum.h"
#include <memory>
#include <vector>

class ArrayCamera;
class Sprite;
class FrustumArray {
  public:
    int coordinateSystem = WebGLCoordinateSystem;
    std::vector<std::shared_ptr<Frustum>> _frustums;
    std::size_t _count = 0;
    FrustumArray& setFromArrayCamera(const ArrayCamera& cameraArray);
    bool intersectsObject(Object3D& object) const;
    bool intersectsSprite(const Sprite& sprite) const;
    bool intersectsSphere(const Sphere& sphere) const;
    bool intersectsBox(const Box3& box) const;
    bool containsPoint(const Vector3& point) const;
    FrustumArray& copy(const FrustumArray& source);
    FrustumArray clone() const;
};
