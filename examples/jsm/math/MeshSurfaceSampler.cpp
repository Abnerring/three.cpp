#include "math/MeshSurfaceSampler.h"
#include "core/BufferAttribute.h"
#include "math/MathUtils.h"
#include "math/Triangle.h"
#include "objects/Mesh.h"
#include <cmath>
#include <cstdint>

namespace {
// The module-level temporaries of the pinned source, one set per owning thread.
thread_local Triangle face;
thread_local Vector3 color;
thread_local Vector2 uva, uvb, uvc;
}

MeshSurfaceSampler::MeshSurfaceSampler(const std::shared_ptr<Mesh>& mesh)
    : MeshSurfaceSampler(mesh ? mesh->geometry : nullptr) {}

MeshSurfaceSampler::MeshSurfaceSampler(const std::shared_ptr<BufferGeometry>& source) {
    geometry = source;
    randomFunction = RandomSource();
    if (!geometry) return;
    indexAttribute = geometry->index;
    positionAttribute = geometry->getAttribute("position");
    normalAttribute = geometry->getAttribute("normal");
    colorAttribute = geometry->getAttribute("color");
    uvAttribute = geometry->getAttribute("uv");
    weightAttribute = nullptr;
}

MeshSurfaceSampler& MeshSurfaceSampler::setWeightAttribute(const std::string& name) {
    weightAttribute = !name.empty() && geometry ? geometry->getAttribute(name) : nullptr;
    return *this;
}

MeshSurfaceSampler& MeshSurfaceSampler::build() {
    const auto index = indexAttribute;
    const auto position = positionAttribute;
    const auto weight = weightAttribute;
    // `indexAttribute.count` / `positionAttribute.count` on a missing attribute is the pinned
    // TypeError; C++ must not dereference the empty handle.
    if (!index && !position) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    const double totalFaces = index ? (index->getCount() / 3.) : (position->getCount() / 3.);
    std::vector<float> faceWeights(static_cast<std::size_t>(totalFaces));
    // Accumulate weights for each mesh face.
    for (double i = 0; i < totalFaces; i += 1) {
        double faceWeight = 1;
        double i0 = 3 * i;
        double i1 = 3 * i + 1;
        double i2 = 3 * i + 2;
        if (index) {
            i0 = index->getX(i0);
            i1 = index->getX(i1);
            i2 = index->getX(i2);
        }
        if (weight)
            faceWeight = weight->getX(i0) + weight->getX(i1) + weight->getX(i2);
        if (!position) throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
        face.a.fromBufferAttribute(*position, i0);
        face.b.fromBufferAttribute(*position, i1);
        face.c.fromBufferAttribute(*position, i2);
        faceWeight *= face.getArea();
        faceWeights[std::size_t(i)] = float(faceWeight);
    }
    // Store cumulative total face weights in an array, where weight index
    // corresponds to face index. The accumulator stays binary64 like the JS number.
    distribution.assign(std::size_t(totalFaces), 0.f);
    double cumulativeTotal = 0;
    for (double i = 0; i < totalFaces; i += 1) {
        cumulativeTotal += faceWeights[std::size_t(i)];
        distribution[std::size_t(i)] = float(cumulativeTotal);
    }
    return *this;
}

MeshSurfaceSampler& MeshSurfaceSampler::setRandomGenerator(RandomSource function) {
    randomFunction = std::move(function);
    return *this;
}

double MeshSurfaceSampler::random() {
    return randomFunction ? randomFunction() : MathUtils::seededRandom();
}

MeshSurfaceSampler& MeshSurfaceSampler::sample(Vector3& targetPosition, Vector3* targetNormal, Color* targetColor,
                                               Vector2* targetUV) {
    const double faceIndex = sampleFaceIndex();
    return sampleFace(faceIndex, targetPosition, targetNormal, targetColor, targetUV);
}

// private
double MeshSurfaceSampler::sampleFaceIndex() {
    if (distribution.empty())
        throw JSTypeError("Cannot read properties of undefined (reading 'length')");
    const double cumulativeTotal = distribution[distribution.size() - 1];
    return binarySearch(random() * cumulativeTotal);
}

double MeshSurfaceSampler::binarySearch(double x) {
    const auto& dist = distribution;
    double start = 0;
    double end = double(dist.size()) - 1;
    double index = -1;
    while (start <= end) {
        const double mid = std::ceil((start + end) / 2);
        if (mid == 0 || (double(dist[std::size_t(mid - 1)]) <= x && double(dist[std::size_t(mid)]) > x)) {
            index = mid;
            break;
        }
        if (x < double(dist[std::size_t(mid)])) end = mid - 1;
        else start = mid + 1;
    }
    return index;
}

MeshSurfaceSampler& MeshSurfaceSampler::sampleFace(double faceIndex, Vector3& targetPosition, Vector3* targetNormal,
                                                   Color* targetColor, Vector2* targetUV) {
    double u = random();
    double v = random();
    if (u + v > 1) {
        u = 1 - u;
        v = 1 - v;
    }
    // get the vertex attribute indices
    const auto index = indexAttribute;
    double i0 = faceIndex * 3;
    double i1 = faceIndex * 3 + 1;
    double i2 = faceIndex * 3 + 2;
    if (index) {
        i0 = index->getX(i0);
        i1 = index->getX(i1);
        i2 = index->getX(i2);
    }
    if (!positionAttribute)
        throw JSTypeError("Cannot read properties of null (reading 'getX')");
    face.a.fromBufferAttribute(*positionAttribute, i0);
    face.b.fromBufferAttribute(*positionAttribute, i1);
    face.c.fromBufferAttribute(*positionAttribute, i2);
    targetPosition.set(0, 0, 0)
        .addScaledVector(face.a, u)
        .addScaledVector(face.b, v)
        .addScaledVector(face.c, 1 - (u + v));
    if (targetNormal != nullptr) {
        if (normalAttribute != nullptr) {
            face.a.fromBufferAttribute(*normalAttribute, i0);
            face.b.fromBufferAttribute(*normalAttribute, i1);
            face.c.fromBufferAttribute(*normalAttribute, i2);
            targetNormal->set(0, 0, 0)
                .addScaledVector(face.a, u)
                .addScaledVector(face.b, v)
                .addScaledVector(face.c, 1 - (u + v))
                .normalize();
        } else {
            face.getNormal(*targetNormal);
        }
    }
    if (targetColor != nullptr && colorAttribute != nullptr) {
        face.a.fromBufferAttribute(*colorAttribute, i0);
        face.b.fromBufferAttribute(*colorAttribute, i1);
        face.c.fromBufferAttribute(*colorAttribute, i2);
        color.set(0, 0, 0)
            .addScaledVector(face.a, u)
            .addScaledVector(face.b, v)
            .addScaledVector(face.c, 1 - (u + v));
        targetColor->r = color.x;
        targetColor->g = color.y;
        targetColor->b = color.z;
    }
    if (targetUV != nullptr && uvAttribute != nullptr) {
        uva.fromBufferAttribute(*uvAttribute, i0);
        uvb.fromBufferAttribute(*uvAttribute, i1);
        uvc.fromBufferAttribute(*uvAttribute, i2);
        targetUV->set(0, 0)
            .addScaledVector(uva, u)
            .addScaledVector(uvb, v)
            .addScaledVector(uvc, 1 - (u + v));
    }
    return *this;
}
