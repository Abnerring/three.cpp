#include "exporters/STLExporter.h"

#include "core/BufferGeometry.h"
#include "core/JSValue.h"
#include "core/VertexAttribute.h"
#include "math/Vector3.h"
#include "objects/Mesh.h"

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
// String concatenation of a number in the pinned exporter; this runtime formats numbers with the
// JavaScript number to string algorithm.
std::string formatNumber(double value) { return JSValue(value).toString(); }
}  // namespace

std::string STLExporter::parse(const Object3D& scene, bool binary) const {
    std::vector<const Mesh*> meshes;
    // The traversal API is not const in C++; the exporter only reads the graph.
    const_cast<Object3D&>(scene).traverse([&meshes](Object3D& object) {
        if (!object.isMesh) return;
        const auto mesh = dynamic_cast<Mesh*>(&object);
        if (mesh && mesh->geometry) meshes.push_back(mesh);
    });

    if (binary) {
        // Binary flavour: 80 byte header, uint32 triangle count, then per triangle the normal and
        // three vertices as float32 little endian plus a zero uint16 attribute byte count.
        double triangleCount = 0;
        for (const auto* mesh : meshes) {
            const auto position =
                mesh->geometry ? mesh->geometry->getAttribute("position") : nullptr;
            const auto index = mesh->geometry ? mesh->geometry->getIndex() : nullptr;
            if (index) {
                triangleCount += index->getCount() / 3;
            } else if (position) {
                triangleCount += position->getCount() / 3;
            }
        }
        std::string bytes(80, '\0');
        const auto appendUint32 = [&bytes](unsigned value) {
            for (int shift = 0; shift < 32; shift += 8) {
                bytes.push_back(static_cast<char>((value >> shift) & 0xFF));
            }
        };
        const auto appendUint16 = [&bytes](unsigned value) {
            bytes.push_back(static_cast<char>(value & 0xFF));
            bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
        };
        const auto appendFloat = [&appendUint32](double value) {
            const float narrowed = static_cast<float>(value);
            unsigned raw = 0;
            std::memcpy(&raw, &narrowed, 4);
            appendUint32(raw);
        };
        appendUint32(static_cast<unsigned>(triangleCount));
        Vector3 a;
        Vector3 b;
        Vector3 c;
        Vector3 cross;
        Vector3 edge;
        Vector3 facetNormal;
        for (const auto* mesh : meshes) {
            const auto position = mesh->geometry->getAttribute("position");
            if (!position) continue;
            const auto index = mesh->geometry->getIndex();
            const auto writeFace = [&](double ia, double ib, double ic) {
                a.fromBufferAttribute(*position, ia);
                b.fromBufferAttribute(*position, ib);
                c.fromBufferAttribute(*position, ic);
                a.applyMatrix4(mesh->matrixWorld);
                b.applyMatrix4(mesh->matrixWorld);
                c.applyMatrix4(mesh->matrixWorld);
                cross.subVectors(c, b);
                edge.subVectors(a, b);
                cross.cross(edge).normalize();
                facetNormal.copy(cross).normalize();
                appendFloat(facetNormal.x);
                appendFloat(facetNormal.y);
                appendFloat(facetNormal.z);
                for (const Vector3* vertex : {&a, &b, &c}) {
                    appendFloat(vertex->x);
                    appendFloat(vertex->y);
                    appendFloat(vertex->z);
                }
                appendUint16(0);
            };
            if (index) {
                for (double j = 0; j < index->getCount(); j += 3) {
                    writeFace(index->getX(j + 0), index->getX(j + 1), index->getX(j + 2));
                }
            } else {
                for (double j = 0; j < position->getCount(); j += 3) {
                    writeFace(j + 0, j + 1, j + 2);
                }
            }
        }
        return bytes;
    }

    std::string output = "solid exported\n";
    Vector3 vertexA;
    Vector3 vertexB;
    Vector3 vertexC;
    Vector3 cb;
    Vector3 ab;
    Vector3 normal;

    const auto writeVertex = [&output](const Vector3& vertex) {
        output += "\t\t\tvertex " + formatNumber(vertex.x) + " " + formatNumber(vertex.y) + " " +
                  formatNumber(vertex.z) + "\n";
    };
    const auto writeFace = [&](double a, double b, double c, const VertexAttribute& position,
                               const Mesh& object) {
        vertexA.fromBufferAttribute(position, a);
        vertexB.fromBufferAttribute(position, b);
        vertexC.fromBufferAttribute(position, c);
        vertexA.applyMatrix4(object.matrixWorld);
        vertexB.applyMatrix4(object.matrixWorld);
        vertexC.applyMatrix4(object.matrixWorld);

        // cb.subVectors( vC, vB ); ab.subVectors( vA, vB ); cb.cross( ab ).normalize(); normal.copy( cb ).normalize()
        cb.subVectors(vertexC, vertexB);
        ab.subVectors(vertexA, vertexB);
        cb.cross(ab).normalize();
        normal.copy(cb).normalize();

        output += "\tfacet normal " + formatNumber(normal.x) + " " + formatNumber(normal.y) + " " +
                  formatNumber(normal.z) + "\n";
        output += "\t\touter loop\n";
        writeVertex(vertexA);
        writeVertex(vertexB);
        writeVertex(vertexC);
        output += "\t\tendloop\n";
        output += "\tendfacet\n";
    };

    for (const auto* mesh : meshes) {
        const auto geometry = mesh->geometry;
        const auto position = geometry->getAttribute("position");
        if (!position) continue;
        const auto index = geometry->getIndex();
        if (index) {
            for (double j = 0; j < index->getCount(); j += 3) {
                writeFace(index->getX(j + 0), index->getX(j + 1), index->getX(j + 2), *position,
                          *mesh);
            }
        } else {
            for (double j = 0; j < position->getCount(); j += 3) {
                writeFace(j + 0, j + 1, j + 2, *position, *mesh);
            }
        }
    }

    output += "endsolid exported\n";
    return output;
}
