#include "geometries/ConvexGeometry.h"
#include "core/BufferAttribute.h"
#include "math/ConvexHull.h"
#include <cstddef>
#include <memory>

ConvexGeometry::ConvexGeometry(const std::vector<Vector3>& points) {
    // buffers

    std::vector<double> vertices;
    std::vector<double> normals;

    ConvexHull convexHull;
    convexHull.setFromPoints(points);

    // generate vertices and normals

    const std::vector<Face*>& faces = convexHull.faces;

    for (std::size_t i = 0; i < faces.size(); i++) {
        Face* face = faces[i];
        HalfEdge* edge = face->edge;

        // we move along a doubly-connected edge list to access all face points (see HalfEdge
        // docs)

        do {
            const Vector3& point = edge->head()->point;

            vertices.push_back(point.x);
            vertices.push_back(point.y);
            vertices.push_back(point.z);
            normals.push_back(face->normal.x);
            normals.push_back(face->normal.y);
            normals.push_back(face->normal.z);

            edge = edge->next;

        } while (edge != face->edge);
    }

    // build geometry

    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(normals, 3));
}
