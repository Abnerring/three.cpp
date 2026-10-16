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
// A JavaScript typed array yields undefined for an index outside [ 0, count ), and
// Vector3.fromBufferAttribute stores that undefined, so the components become NaN. Reading an
// out-of-range vertex index is therefore NaN, not a memory access, and the native port has to
// reproduce that: a position count that is not a multiple of three reaches exactly this case.
template <class Target> void readInto(Target& target, const VertexAttribute& attribute, double index) {
    if (!(index >= 0) || index >= attribute.getCount()) {
        target.set(std::nan(""), std::nan(""), std::nan(""));
        return;
    }
    target.fromBufferAttribute(attribute, index);
}
void readInto(Vector2& target, const VertexAttribute& attribute, double index) {
    if (!(index >= 0) || index >= attribute.getCount()) {
        target.set(std::nan(""), std::nan(""));
        return;
    }
    target.fromBufferAttribute(attribute, index);
}
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
    // `new Float32Array( totalFaces )` applies ToIndex: the length is the floored value, so a
    // position count that is not a multiple of three leaves a shorter array than the loop below
    // walks. A typed array silently ignores an out-of-range write, and the native port reproduces
    // that instead of writing past the end of the vector.
    const std::size_t weightCount = totalFaces > 0 ? std::size_t(totalFaces) : 0;
    std::vector<float> faceWeights(weightCount);
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
        readInto(face.a, *position, i0);
        readInto(face.b, *position, i1);
        readInto(face.c, *position, i2);
        faceWeight *= face.getArea();
        if (std::size_t(i) < faceWeights.size()) faceWeights[std::size_t(i)] = float(faceWeight);
    }
    // Store cumulative total face weights in an array, where weight index
    // corresponds to face index. The accumulator stays binary64 like the JS number.
    distribution.assign(std::size_t(totalFaces), 0.f);
    double cumulativeTotal = 0;
    for (double i = 0; i < totalFaces; i += 1) {
        // The same ToIndex rule applies here: a JS read past the end of the weight array is
        // undefined, so the running total becomes NaN and the write into the distribution is
        // ignored - it must not read or write outside the native storage either.
        const bool inWeights = std::size_t(i) < faceWeights.size();
        cumulativeTotal += inWeights ? double(faceWeights[std::size_t(i)]) : std::nan("");
        if (std::size_t(i) < distribution.size()) distribution[std::size_t(i)] = float(cumulativeTotal);
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
    // `distribution[ distribution.length - 1 ]` on an empty typed array is undefined, so the
    // product is NaN and the search returns -1; the pinned class never throws on this path.
    const double cumulativeTotal = distribution.empty() ? std::nan("") : double(distribution[distribution.size() - 1]);
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
    if (faceIndex < 0) {
        // Negative indices read `undefined` from the attributes upstream, so every requested
        // target becomes NaN; nothing is thrown and no memory outside the attributes is touched.
        targetPosition.set(std::nan(""), std::nan(""), std::nan(""));
        if (targetNormal != nullptr) targetNormal->set(std::nan(""), std::nan(""), std::nan(""));
        if (targetColor != nullptr && colorAttribute != nullptr) {
            targetColor->r = std::nan("");
            targetColor->g = std::nan("");
            targetColor->b = std::nan("");
        }
        if (targetUV != nullptr && uvAttribute != nullptr) targetUV->set(std::nan(""), std::nan(""));
        return *this;
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
    readInto(face.a, *positionAttribute, i0);
    readInto(face.b, *positionAttribute, i1);
    readInto(face.c, *positionAttribute, i2);
    targetPosition.set(0, 0, 0)
        .addScaledVector(face.a, u)
        .addScaledVector(face.b, v)
        .addScaledVector(face.c, 1 - (u + v));
    if (targetNormal != nullptr) {
        if (normalAttribute != nullptr) {
            readInto(face.a, *normalAttribute, i0);
            readInto(face.b, *normalAttribute, i1);
            readInto(face.c, *normalAttribute, i2);
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
        readInto(face.a, *colorAttribute, i0);
        readInto(face.b, *colorAttribute, i1);
        readInto(face.c, *colorAttribute, i2);
        color.set(0, 0, 0)
            .addScaledVector(face.a, u)
            .addScaledVector(face.b, v)
            .addScaledVector(face.c, 1 - (u + v));
        targetColor->r = color.x;
        targetColor->g = color.y;
        targetColor->b = color.z;
    }
    if (targetUV != nullptr && uvAttribute != nullptr) {
        readInto(uva, *uvAttribute, i0);
        readInto(uvb, *uvAttribute, i1);
        readInto(uvc, *uvAttribute, i2);
        targetUV->set(0, 0)
            .addScaledVector(uva, u)
            .addScaledVector(uvb, v)
            .addScaledVector(uvc, 1 - (u + v));
    }
    return *this;
}
