#pragma once
#include "core/JsonValue.h"
#include "core/Intersection.h"
#include "core/Layers.h"
#include "core/SharedArray.h"
#include "math/Ray.h"
#include <limits>
#include <vector>

class Camera;
class Object3D;
class Raycaster {
  public:
    Ray ray;
    double near, far;
    Camera* camera = nullptr;
    Layers layers;
    JsonValue params = JsonValue::Object{{"Mesh", JsonValue::Object{}},
                                         {"Line", JsonValue::Object{{"threshold", 1}}},
                                         {"LOD", JsonValue::Object{}},
                                         {"Points", JsonValue::Object{{"threshold", 1}}},
                                         {"Sprite", JsonValue::Object{}}};
    Raycaster(const Vector3& origin = Vector3(), const Vector3& direction = Vector3(0, 0, -1),
              double near = 0, double far = INFINITY);
    void set(const Vector3& origin, const Vector3& direction);
    void setFromCamera(const Vector2& coordinates, Camera& camera);
    Raycaster& setFromXRController(const Object3D& controller);
    std::vector<Intersection>& intersectObject(Object3D& object, bool recursive,
                                               std::vector<Intersection>& target) const;
    std::vector<Intersection> intersectObject(Object3D& object, bool recursive = true) const;
    std::vector<Intersection>&
    intersectObjects(SharedArray<std::shared_ptr<Object3D>> objects, bool recursive,
                     std::vector<Intersection>& target) const;
    std::vector<Intersection>
    intersectObjects(SharedArray<std::shared_ptr<Object3D>> objects,
                     bool recursive = true) const;

  private:
    std::shared_ptr<Camera> cameraOwner_;
};
