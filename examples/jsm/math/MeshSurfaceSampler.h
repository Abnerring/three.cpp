#pragma once
#include "core/BufferGeometry.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Mesh;

// Native port of the pinned examples/jsm/math/MeshSurfaceSampler.js.
//
// The upstream class reads `Math.random`; the native port keeps the same rule with an
// injectable source. An empty std::function selects the native default, which is the
// deterministic `MathUtils::seededRandom()` generator, because there is no `Math.random` here.
//
// Differences that follow from the language: the optional targets of `sample()` are pointers
// (nullptr means "not requested", exactly like `targetNormal === undefined` upstream), and the
// module-level temporaries are thread_local. `distribution` is the Float32Array of the pinned
// module (float32 rounding is observable in the binary search), while the running total that
// fills it stays binary64 like the JS accumulator.
class MeshSurfaceSampler {
  public:
    /** A function returning one random number in [ 0, 1 ); empty selects the native default. */
    using RandomSource = std::function<double()>;

    explicit MeshSurfaceSampler(const std::shared_ptr<Mesh>& mesh);
    // The upstream constructor reads the mesh geometry; this overload is for a bare geometry.
    explicit MeshSurfaceSampler(const std::shared_ptr<BufferGeometry>& geometry);

    MeshSurfaceSampler& setWeightAttribute(const std::string& name);
    MeshSurfaceSampler& build();
    MeshSurfaceSampler& setRandomGenerator(RandomSource randomFunction);
    MeshSurfaceSampler& sample(Vector3& targetPosition, Vector3* targetNormal = nullptr,
        Color* targetColor = nullptr, Vector2* targetUV = nullptr);

    std::shared_ptr<BufferGeometry> geometry;
    RandomSource randomFunction;
    std::shared_ptr<VertexAttribute> indexAttribute, positionAttribute, normalAttribute, colorAttribute,
        uvAttribute, weightAttribute;
    std::vector<float> distribution;

  private:
    double sampleFaceIndex();
    double binarySearch(double x);
    MeshSurfaceSampler& sampleFace(double faceIndex, Vector3& targetPosition, Vector3* targetNormal,
        Color* targetColor, Vector2* targetUV);
    double random();
};
