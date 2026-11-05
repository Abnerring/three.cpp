#pragma once

#include "math/Matrix4.h"
#include "objects/Mesh.h"

#include "math/Color.h"

#include <memory>

class Plane;
class Vector4;

// Native port of examples/jsm/objects/ShadowMesh.js from the pinned three.js revision
// (see upstream.json).
//
// A projected shadow mesh: it reuses the source mesh geometry and, on update, multiplies a plane
// projection matrix built from the plane and the light position by the source world matrix. The
// material uses the stencil buffer so the projected quads do not double blend.
//
// The pinned class stores `this.meshMatrix = mesh.matrixWorld`, that is a reference to the source
// mesh matrix, not a copy, so later world updates of the source are picked up. C++ mirrors that by
// keeping a pointer; the source mesh must outlive the ShadowMesh.
class ShadowMesh : public Mesh {
  public:
    const bool isShadowMesh = true;
    // Reference to the source mesh world matrix, like the pinned `meshMatrix` field.
    Matrix4* meshMatrix = nullptr;

    explicit ShadowMesh(const std::shared_ptr<Mesh>& mesh);

    void update(const Plane& plane, const Vector4& lightPosition4D);
};
