#pragma once
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <memory>
#include <optional>

class Object3D;
struct Intersection {
    struct Face {
        std::size_t a = 0, b = 0, c = 0;
        Vector3 normal;
        std::size_t materialIndex = 0;
    };
    double distance = 0;
    Vector3 point;
    Object3D* object = nullptr;
    // A JS intersection owns its object reference. Stack-owned C++ objects use object only.
    std::shared_ptr<Object3D> objectOwner;
    std::optional<Vector2> uv, uv1;
    std::optional<Vector3> normal, barycoord;
    std::optional<Face> face;
    std::optional<std::size_t> faceIndex;
    std::optional<double> index, instanceId, distanceToRay;
};
