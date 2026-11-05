#include "exporters/STLExporter.h"

#include "core/BufferGeometry.h"
#include "core/JSValue.h"
#include "core/VertexAttribute.h"
#include "math/Vector3.h"
#include "objects/Mesh.h"

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
    if (binary) {
        throw std::runtime_error("STLExporter: binary output is not ported yet");
    }

    std::vector<const Mesh*> meshes;
    // The traversal API is not const in C++; the exporter only reads the graph.
    const_cast<Object3D&>(scene).traverse([&meshes](Object3D& object) {
        if (!object.isMesh) return;
        const auto mesh = dynamic_cast<Mesh*>(&object);
        if (mesh && mesh->geometry) meshes.push_back(mesh);
    });

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
