#include "exporters/OBJExporter.h"

#include "core/BufferGeometry.h"
#include "core/JSValue.h"
#include "core/VertexAttribute.h"
#include "math/Matrix3.h"
#include "math/Vector3.h"
#include "objects/Mesh.h"

#include <string>
#include <vector>

namespace {
std::string formatNumber(double value) { return JSValue(value).toString(); }
}  // namespace

std::string OBJExporter::parse(const Object3D& object) const {
    std::string output;
    double indexVertex = 0;
    double indexVertexUvs = 0;
    double indexNormals = 0;
    Vector3 vertex;
    Vector3 normal;

    const auto parseMesh = [&](const Mesh& mesh) {
        double nbVertex = 0;
        double nbNormals = 0;
        double nbVertexUvs = 0;
        const auto geometry = mesh.geometry;
        const auto vertices = geometry ? geometry->getAttribute("position") : nullptr;
        const auto normals = geometry ? geometry->getAttribute("normal") : nullptr;
        const auto uvs = geometry ? geometry->getAttribute("uv") : nullptr;
        const auto indices = geometry ? geometry->getIndex() : nullptr;

        output += "o " + mesh.name + "\n";
        if (mesh.material && !mesh.material->name.empty()) {
            output += "usemtl " + mesh.material->name + "\n";
        }
        if (vertices) {
            for (double i = 0; i < vertices->getCount(); ++i, ++nbVertex) {
                vertex.fromBufferAttribute(*vertices, i);
                vertex.applyMatrix4(mesh.matrixWorld);
                output += "v " + formatNumber(vertex.x) + " " + formatNumber(vertex.y) + " " +
                          formatNumber(vertex.z) + "\n";
            }
        }
        if (uvs) {
            for (double i = 0; i < uvs->getCount(); ++i, ++nbVertexUvs) {
                output += "vt " + formatNumber(uvs->getX(i)) + " " + formatNumber(uvs->getY(i)) + "\n";
            }
        }
        if (normals) {
            Matrix3 normalMatrixWorld;
            normalMatrixWorld.getNormalMatrix(mesh.matrixWorld);
            for (double i = 0; i < normals->getCount(); ++i, ++nbNormals) {
                normal.fromBufferAttribute(*normals, i);
                normal.applyMatrix3(normalMatrixWorld).normalize();
                output += "vn " + formatNumber(normal.x) + " " + formatNumber(normal.y) + " " +
                          formatNumber(normal.z) + "\n";
            }
        }

        const bool hasNormalsOrUvs = static_cast<bool>(normals) || static_cast<bool>(uvs);
        const auto faceComponent = [&](double indexOffset) {
            std::string face = formatNumber(indexVertex + indexOffset);
            if (hasNormalsOrUvs) {
                face += "/";
                if (uvs) face += formatNumber(indexVertexUvs + indexOffset);
                if (normals) face += "/" + formatNumber(indexNormals + indexOffset);
            }
            return face;
        };
        if (indices) {
            for (double i = 0; i < indices->getCount(); i += 3) {
                std::string face;
                for (int m = 0; m < 3; ++m) {
                    const double j = indices->getX(i + m) + 1;
                    if (m > 0) face += " ";
                    face += faceComponent(j);
                }
                output += "f " + face + "\n";
            }
        } else if (vertices) {
            for (double i = 0; i < vertices->getCount(); i += 3) {
                std::string face;
                for (int m = 0; m < 3; ++m) {
                    const double j = i + m + 1;
                    if (m > 0) face += " ";
                    face += faceComponent(j);
                }
                output += "f " + face + "\n";
            }
        }
        indexVertex += nbVertex;
        indexVertexUvs += nbVertexUvs;
        indexNormals += nbNormals;
    };

    const_cast<Object3D&>(object).traverse([&](Object3D& child) {
        if (child.isMesh) {
            if (const auto mesh = dynamic_cast<Mesh*>(&child)) parseMesh(*mesh);
        }
    });
    return output;
}
