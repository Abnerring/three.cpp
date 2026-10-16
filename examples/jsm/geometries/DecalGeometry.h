#pragma once
#include "core/BufferGeometry.h"
#include "math/Euler.h"
#include "math/Vector3.h"
#include <memory>
#include <optional>
#include <vector>

class Mesh;

// Native port of the pinned examples/jsm/geometries/DecalGeometry.js, including the
// exported DecalVertex helper. A missing normal is `null` upstream and std::nullopt here.
class DecalVertex {
  public:
    Vector3 position;
    std::optional<Vector3> normal;

    explicit DecalVertex(Vector3 position = {}, std::optional<Vector3> normal = std::nullopt)
        : position(std::move(position)), normal(std::move(normal)) {}
    DecalVertex clone() const {
        const std::optional<Vector3> copied = normal ? std::optional<Vector3>(normal->clone()) : std::nullopt;
        return DecalVertex(position.clone(), copied);
    }
};

// Clips the geometry of `mesh` with a box in projector space, using the mesh's current
// matrixWorld exactly like the pinned constructor (which never updates it).
class DecalGeometry : public BufferGeometry {
  public:
    using Vertices = std::vector<DecalVertex>;

    explicit DecalGeometry(const std::shared_ptr<Mesh>& mesh = nullptr, const Vector3& position = {},
                           const Euler& orientation = {}, const Vector3& size = Vector3(1, 1, 1));

  private:
    void generate(const std::shared_ptr<Mesh>& mesh, const Matrix4& projectorMatrix,
                  const Matrix4& projectorMatrixInverse, const Matrix3& normalMatrix, const Vector3& size,
                  std::vector<double>& vertices, std::vector<double>& normals, std::vector<double>& uvs);
};
