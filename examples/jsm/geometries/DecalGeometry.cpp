#include "geometries/DecalGeometry.h"
#include "core/BufferAttribute.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "objects/Mesh.h"
#include <cmath>
#include <cstddef>

namespace {
// The helpers of the pinned constructor, which close over the projector state.
void pushDecalVertex(const std::shared_ptr<Mesh>& mesh, const Matrix4& projectorMatrixInverse,
                     const Matrix3& normalMatrix, DecalGeometry::Vertices& decalVertices, Vector3& vertex,
                     const Vector3* normal) {
    // transform the vertex to world space, then to projector space
    vertex.applyMatrix4(mesh->matrixWorld);
    vertex.applyMatrix4(projectorMatrixInverse);
    if (normal) {
        Vector3 copied = *normal;
        copied.applyNormalMatrix(normalMatrix);
        decalVertices.emplace_back(vertex.clone(), std::optional<Vector3>(std::move(copied)));
    } else {
        decalVertices.emplace_back(vertex.clone(), std::nullopt);
    }
}

void clipInto(const DecalVertex& v0, const DecalVertex& v1, const Vector3& p, double s, DecalVertex& target) {
    const double d0 = v0.position.dot(p) - s;
    const double d1 = v1.position.dot(p) - s;
    const double s0 = d0 / (d0 - d1);
    const Vector3 position(v0.position.x + s0 * (v1.position.x - v0.position.x),
        v0.position.y + s0 * (v1.position.y - v0.position.y),
        v0.position.z + s0 * (v1.position.z - v0.position.z));
    std::optional<Vector3> normal;
    if (v0.normal && v1.normal) {
        normal = Vector3(v0.normal->x + s0 * (v1.normal->x - v0.normal->x),
            v0.normal->y + s0 * (v1.normal->y - v0.normal->y),
            v0.normal->z + s0 * (v1.normal->z - v0.normal->z));
    }
    target = DecalVertex(position, normal);
}

DecalGeometry::Vertices clipGeometry(const DecalGeometry::Vertices& inVertices, const Vector3& plane,
                                     const Vector3& size) {
    DecalGeometry::Vertices outVertices;
    const double s = 0.5 * std::abs(size.dot(plane));
    // a single iteration clips one face,
    // which consists of three consecutive 'DecalVertex' objects
    for (std::size_t i = 0; i < inVertices.size(); i += 3) {
        // A trailing partial face is the upstream TypeError on `undefined.position`.
        if (i + 2 >= inVertices.size())
            throw JSTypeError("Cannot read properties of undefined (reading 'position')");
        DecalVertex nV1, nV2, nV3, nV4;
        const double d1 = inVertices[i + 0].position.dot(plane) - s;
        const double d2 = inVertices[i + 1].position.dot(plane) - s;
        const double d3 = inVertices[i + 2].position.dot(plane) - s;
        const bool v1Out = d1 > 0;
        const bool v2Out = d2 > 0;
        const bool v3Out = d3 > 0;
        // calculate, how many vertices of the face lie outside of the clipping plane
        const int total = (v1Out ? 1 : 0) + (v2Out ? 1 : 0) + (v3Out ? 1 : 0);
        switch (total) {
            case 0: {
                // the entire face lies inside of the plane, no clipping needed
                outVertices.push_back(inVertices[i]);
                outVertices.push_back(inVertices[i + 1]);
                outVertices.push_back(inVertices[i + 2]);
                break;
            }
            case 1: {
                // one vertex lies outside of the plane, perform clipping
                if (v1Out) {
                    nV1 = inVertices[i + 1];
                    nV2 = inVertices[i + 2];
                    clipInto(inVertices[i], nV1, plane, s, nV3);
                    clipInto(inVertices[i], nV2, plane, s, nV4);
                }
                if (v2Out) {
                    nV1 = inVertices[i];
                    nV2 = inVertices[i + 2];
                    clipInto(inVertices[i + 1], nV1, plane, s, nV3);
                    clipInto(inVertices[i + 1], nV2, plane, s, nV4);
                    outVertices.push_back(nV3);
                    outVertices.push_back(nV2.clone());
                    outVertices.push_back(nV1.clone());
                    outVertices.push_back(nV2.clone());
                    outVertices.push_back(nV3.clone());
                    outVertices.push_back(nV4);
                    break;
                }
                if (v3Out) {
                    nV1 = inVertices[i];
                    nV2 = inVertices[i + 1];
                    clipInto(inVertices[i + 2], nV1, plane, s, nV3);
                    clipInto(inVertices[i + 2], nV2, plane, s, nV4);
                }
                outVertices.push_back(nV1.clone());
                outVertices.push_back(nV2.clone());
                outVertices.push_back(nV3);
                outVertices.push_back(nV4);
                outVertices.push_back(nV3.clone());
                outVertices.push_back(nV2.clone());
                break;
            }
            case 2: {
                // two vertices lies outside of the plane, perform clipping
                if (!v1Out) {
                    nV1 = inVertices[i].clone();
                    clipInto(nV1, inVertices[i + 1], plane, s, nV2);
                    clipInto(nV1, inVertices[i + 2], plane, s, nV3);
                    outVertices.push_back(nV1);
                    outVertices.push_back(nV2);
                    outVertices.push_back(nV3);
                }
                if (!v2Out) {
                    nV1 = inVertices[i + 1].clone();
                    clipInto(nV1, inVertices[i + 2], plane, s, nV2);
                    clipInto(nV1, inVertices[i], plane, s, nV3);
                    outVertices.push_back(nV1);
                    outVertices.push_back(nV2);
                    outVertices.push_back(nV3);
                }
                if (!v3Out) {
                    nV1 = inVertices[i + 2].clone();
                    clipInto(nV1, inVertices[i], plane, s, nV2);
                    clipInto(nV1, inVertices[i + 1], plane, s, nV3);
                    outVertices.push_back(nV1);
                    outVertices.push_back(nV2);
                    outVertices.push_back(nV3);
                }
                break;
            }
            case 3: {
                // the entire face lies outside of the plane, so let's discard the corresponding vertices
                break;
            }
        }
    }
    return outVertices;
}
}

DecalGeometry::DecalGeometry(const std::shared_ptr<Mesh>& mesh, const Vector3& position, const Euler& orientation,
                             const Vector3& size) {
    // buffers
    std::vector<double> vertices, normals, uvs;
    // helpers
    Matrix3 normalMatrix;
    if (mesh) normalMatrix.getNormalMatrix(mesh->matrixWorld);
    // this matrix represents the transformation of the decal projector
    Matrix4 projectorMatrix;
    projectorMatrix.makeRotationFromEuler(orientation);
    projectorMatrix.setPosition(position);
    Matrix4 projectorMatrixInverse;
    projectorMatrixInverse.copy(projectorMatrix).invert();
    // generate buffers
    generate(mesh, projectorMatrix, projectorMatrixInverse, normalMatrix, size, vertices, normals, uvs);
    // build geometry
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
    if (!normals.empty()) setAttribute("normal", std::make_shared<Float32BufferAttribute>(normals, 3));
}

void DecalGeometry::generate(const std::shared_ptr<Mesh>& mesh, const Matrix4& projectorMatrix,
                             const Matrix4& projectorMatrixInverse, const Matrix3& normalMatrix, const Vector3& size,
                             std::vector<double>& vertices, std::vector<double>& normals, std::vector<double>& uvs) {
    Vertices decalVertices;
    Vector3 vertex, normal;
    // handle different geometry types
    const auto geometry = mesh ? mesh->geometry : nullptr;
    const auto positionAttribute = geometry ? geometry->getAttribute("position") : nullptr;
    const auto normalAttribute = geometry ? geometry->getAttribute("normal") : nullptr;
    // first, create an array of 'DecalVertex' objects
    // three consecutive 'DecalVertex' objects represent a single face
    //
    // this data structure will be later used to perform the clipping
    if (geometry && geometry->index != nullptr) {
        // indexed BufferGeometry
        const auto& indexAttribute = geometry->index;
        for (double i = 0; i < indexAttribute->getCount(); i += 1) {
            vertex.fromBufferAttribute(*positionAttribute, indexAttribute->getX(i));
            if (normalAttribute) {
                normal.fromBufferAttribute(*normalAttribute, indexAttribute->getX(i));
                pushDecalVertex(mesh, projectorMatrixInverse, normalMatrix, decalVertices, vertex, &normal);
            } else {
                pushDecalVertex(mesh, projectorMatrixInverse, normalMatrix, decalVertices, vertex, nullptr);
            }
        }
    } else {
        if (!positionAttribute) return; // empty geometry
        // non-indexed BufferGeometry
        for (double i = 0; i < positionAttribute->getCount(); i += 1) {
            vertex.fromBufferAttribute(*positionAttribute, i);
            if (normalAttribute) {
                normal.fromBufferAttribute(*normalAttribute, i);
                pushDecalVertex(mesh, projectorMatrixInverse, normalMatrix, decalVertices, vertex, &normal);
            } else {
                pushDecalVertex(mesh, projectorMatrixInverse, normalMatrix, decalVertices, vertex, nullptr);
            }
        }
    }
    // second, clip the geometry so that it doesn't extend out from the projector
    decalVertices = clipGeometry(decalVertices, Vector3(1, 0, 0), size);
    decalVertices = clipGeometry(decalVertices, Vector3(-1, 0, 0), size);
    decalVertices = clipGeometry(decalVertices, Vector3(0, 1, 0), size);
    decalVertices = clipGeometry(decalVertices, Vector3(0, -1, 0), size);
    decalVertices = clipGeometry(decalVertices, Vector3(0, 0, 1), size);
    decalVertices = clipGeometry(decalVertices, Vector3(0, 0, -1), size);
    // third, generate final vertices, normals and uvs
    for (std::size_t i = 0; i < decalVertices.size(); ++i) {
        DecalVertex& decalVertex = decalVertices[i];
        // create texture coordinates (we are still in projector space)
        uvs.push_back(0.5 + (decalVertex.position.x / size.x));
        uvs.push_back(0.5 + (decalVertex.position.y / size.y));
        // transform the vertex back to world space
        decalVertex.position.applyMatrix4(projectorMatrix);
        // now create vertex and normal buffer data
        vertices.push_back(decalVertex.position.x);
        vertices.push_back(decalVertex.position.y);
        vertices.push_back(decalVertex.position.z);
        if (decalVertex.normal) {
            normals.push_back(decalVertex.normal->x);
            normals.push_back(decalVertex.normal->y);
            normals.push_back(decalVertex.normal->z);
        }
    }
}
