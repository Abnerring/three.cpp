#include "catch.hpp"
#include "core/JSValue.h"
#include "geometries/BoxGeometry.h"
#include "math/ConvexHull.h"
#include "math/Ray.h"
#include "objects/Mesh.h"
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

// Reference values in this file were produced by running the pinned three.js module
// examples/jsm/math/ConvexHull.js under the pinned Node oracle.
namespace {

// The order of the pinned reference run: x outer, then y, then z.
std::vector<Vector3> cubeCorners() {
    std::vector<Vector3> points;
    for (const double x : {-1., 1.})
        for (const double y : {-1., 1.})
            for (const double z : {-1., 1.}) points.push_back(Vector3(x, y, z));
    return points;
}

int vertexIndex(const ConvexHull& hull, const VertexNode* vertex) {
    if (vertex == nullptr) return -1;
    for (std::size_t i = 0; i < hull.vertices.size(); i++)
        if (hull.vertices[i] == vertex) return int(i);
    return -1;
}
int faceIndex(const ConvexHull& hull, const Face* face) {
    if (face == nullptr) return -1;
    for (std::size_t i = 0; i < hull.faces.size(); i++)
        if (hull.faces[i] == face) return int(i);
    return -1;
}
// The three destination vertices of a face in half-edge order, which is the order
// ConvexGeometry walks.
std::vector<int> faceVertexIndices(const ConvexHull& hull, const Face* face) {
    std::vector<int> indices;
    const HalfEdge* edge = face->edge;
    for (std::size_t guard = 0; guard < 8; guard++) {
        indices.push_back(vertexIndex(hull, edge->head()));
        edge = edge->next;
        if (edge == face->edge) break;
    }
    return indices;
}
// Walks a linked list of the hull through `next`, like the pinned source does.
std::vector<int> vertexChain(const ConvexHull& hull, const VertexNode* head) {
    std::vector<int> chain;
    const VertexNode* vertex = head;
    for (std::size_t guard = 0; vertex != nullptr && guard < 64; guard++) {
        chain.push_back(vertexIndex(hull, vertex));
        vertex = vertex->next;
    }
    return chain;
}
double cubeTolerance() {
    return 3 * std::numeric_limits<double>::epsilon() * (1. + 1. + 1.);
}
// Walks a VertexList through `next`, which is how the pinned source traverses it.
std::vector<VertexNode*> vertexChainOf(const VertexList& list) {
    std::vector<VertexNode*> chain;
    VertexNode* vertex = list.head;
    for (std::size_t guard = 0; vertex != nullptr && guard < 64; guard++) {
        chain.push_back(vertex);
        vertex = vertex->next;
    }
    return chain;
}

} // namespace

TEST_CASE("ConvexHull constructor defaults and makeEmpty", "[convexhull]") {
    ConvexHull hull;

    SECTION("the constructor defaults of the pinned class") {
        REQUIRE(hull.tolerance == -1.);
        REQUIRE(hull.faces.empty());
        REQUIRE(hull.newFaces.empty());
        REQUIRE(hull.vertices.empty());
        REQUIRE(hull.assigned.isEmpty());
        REQUIRE(hull.unassigned.isEmpty());
        REQUIRE(hull.assigned.first() == nullptr);
        REQUIRE(hull.assigned.last() == nullptr);
        REQUIRE(hull.unassigned.first() == nullptr);
        REQUIRE(hull.unassigned.last() == nullptr);
    }

    SECTION("makeEmpty returns this and drops the faces and vertices") {
        hull.setFromPoints(cubeCorners());
        REQUIRE(hull.faces.size() == 12);
        REQUIRE(&hull.makeEmpty() == &hull);
        REQUIRE(hull.faces.empty());
        REQUIRE(hull.vertices.empty());
    }
}

TEST_CASE("ConvexHull setFromPoints builds the cube hull", "[convexhull]") {
    ConvexHull hull;
    const std::vector<Vector3> points = cubeCorners();
    REQUIRE(&hull.setFromPoints(points) == &hull);

    SECTION("face and vertex counts plus the tolerance of the pinned module") {
        REQUIRE(hull.vertices.size() == 8);
        REQUIRE(hull.faces.size() == 12);
        REQUIRE(hull.tolerance == cubeTolerance());
        // _cleanup() empties the working lists and newFaces stays empty.
        REQUIRE(hull.newFaces.empty());
        REQUIRE(hull.assigned.isEmpty());
        REQUIRE(hull.unassigned.isEmpty());
    }

    SECTION("the faces list the same vertices in the same order") {
        const std::vector<std::vector<int>> expected = {{1, 0, 4}, {1, 3, 0}, {6, 4, 0}, {7, 3, 1},
                                                        {7, 4, 6}, {7, 6, 3}, {2, 3, 6}, {2, 6, 0},
                                                        {2, 0, 3}, {5, 4, 7}, {5, 7, 1}, {5, 1, 4}};
        REQUIRE(hull.faces.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); i++)
            REQUIRE(faceVertexIndices(hull, hull.faces[i]) == expected[i]);
    }

    SECTION("the face properties of the pinned module") {
        const Face* face = hull.faces[0];
        REQUIRE(face->normal.x == 0.);
        REQUIRE(face->normal.y == -1.);
        REQUIRE(face->normal.z == 0.);
        REQUIRE(face->midpoint.x == -1. / 3.);
        REQUIRE(face->midpoint.y == -1.);
        REQUIRE(face->midpoint.z == -1. / 3.);
        REQUIRE(face->area == 2.);
        REQUIRE(face->constant == 1.);
        REQUIRE(face->mark == Face::Visible);
        REQUIRE(face->outside == nullptr);

        // Every face of the cube hull is a unit normal with |constant| = 1 and area 2, and all
        // of them stay visible after _reindexFaces().
        for (const Face* other : hull.faces) {
            REQUIRE(other->mark == Face::Visible);
            const double length = other->normal.length();
            REQUIRE(std::fabs(length - 1.) < 1e-15);
            REQUIRE(other->area == 2.);
            REQUIRE(std::fabs(other->constant) == 1.);
            REQUIRE(other->constant == other->normal.dot(other->midpoint));
            REQUIRE(other->outside == nullptr);
            // The hull is convex and every face points away from the center.
            REQUIRE(other->distanceToPoint(other->midpoint) == 0.);
            REQUIRE(other->distanceToPoint(Vector3()) == -1.);
        }
    }

    SECTION("every half edge keeps the pinned links") {
        std::size_t edgeCount = 0;
        for (const Face* face : hull.faces) {
            const HalfEdge* edge = face->edge;
            for (std::size_t guard = 0; guard < 8; guard++) {
                edgeCount++;
                REQUIRE(edge->face == face);
                REQUIRE(edge->head() == edge->vertex);
                REQUIRE(edge->tail() == edge->prev->vertex);
                REQUIRE(edge->next->prev == edge);
                REQUIRE(edge->prev->next == edge);
                // A face always consists of three edges.
                REQUIRE(edge->next->next->next == edge);
                REQUIRE(edge->twin != nullptr);
                REQUIRE(edge->twin->twin == edge);
                REQUIRE(edge->twin->face != face);
                REQUIRE(edge->twin->head() == edge->tail());
                REQUIRE(edge->twin->tail() == edge->head());
                // Cube corners differ by exactly 2 on one or two axes.
                const double lengthSquared = edge->lengthSquared();
                REQUIRE((lengthSquared == 4. || lengthSquared == 8.));
                REQUIRE(edge->length() == std::sqrt(lengthSquared));
                REQUIRE(edge->length() == edge->tail()->point.distanceTo(edge->head()->point));
                edge = edge->next;
                if (edge == face->edge) break;
            }
        }
        REQUIRE(edgeCount == 36);
    }

    SECTION("the vertices keep the pinned stale links of the source") {
        // _cleanup() clears the working lists without resetting the nodes, so some vertices
        // keep the 'next' link and the deleted face reference they had while they were
        // assigned. The pinned module behaves the same way; both columns below come from the
        // reference run.
        REQUIRE(hull.vertices.size() == 8);
        const bool faceIsNull[8] = {true, true, false, true, true, false, false, false};
        const bool nextIsNull[8] = {true, true, false, true, true, true, false, false};
        for (std::size_t i = 0; i < hull.vertices.size(); i++) {
            const VertexNode* vertex = hull.vertices[i];
            REQUIRE(vertex->prev == nullptr);
            REQUIRE((vertex->face == nullptr) == faceIsNull[i]);
            REQUIRE((vertex->next == nullptr) == nextIsNull[i]);
            // A surviving reference points at a face that _reindexFaces() dropped.
            REQUIRE(faceIndex(hull, vertex->face) == -1);
        }
        REQUIRE(vertexIndex(hull, hull.vertices[2]->next) == 5);
        REQUIRE(vertexIndex(hull, hull.vertices[6]->next) == 2);
        REQUIRE(vertexIndex(hull, hull.vertices[7]->next) == 5);
    }
}

TEST_CASE("ConvexHull containsPoint", "[convexhull]") {
    ConvexHull hull;
    hull.setFromPoints(cubeCorners());

    SECTION("points inside, on the surface and outside the pinned cube hull") {
        REQUIRE(hull.containsPoint(Vector3(0, 0, 0)));
        REQUIRE(hull.containsPoint(Vector3(1, 1, 1)));
        REQUIRE_FALSE(hull.containsPoint(Vector3(2, 0, 0)));
        // The tolerance of the pinned module is 3 * Number.EPSILON * 6, so a point 1e-12
        // outside the cube is still reported as outside.
        REQUIRE_FALSE(hull.containsPoint(Vector3(1 + 1e-12, 0, 0)));
        REQUIRE(hull.containsPoint(Vector3(1.0000000000000002, 0, 0)));
    }

    SECTION("an empty hull contains every point") {
        const ConvexHull empty;
        REQUIRE(empty.containsPoint(Vector3(0, 0, 0)));
        REQUIRE(empty.containsPoint(Vector3(1e6, -1e6, 0)));
    }
}

TEST_CASE("ConvexHull intersectRay and intersectsRay", "[convexhull]") {
    ConvexHull hull;
    hull.setFromPoints(cubeCorners());

    SECTION("the intersection point of the pinned reference run") {
        Vector3 target;
        Vector3* hit = hull.intersectRay(Ray(Vector3(0, 0, -5), Vector3(0, 0, 1)), target);
        REQUIRE(hit == &target);
        REQUIRE(target.x == 0.);
        REQUIRE(target.y == 0.);
        REQUIRE(target.z == -1.);

        REQUIRE(hull.intersectRay(Ray(Vector3(0, 0, 5), Vector3(0, 0, -1)), target) == &target);
        REQUIRE(target.z == 1.);
        REQUIRE(hull.intersectRay(Ray(Vector3(0, 0, 0), Vector3(1, 0, 0)), target) == &target);
        REQUIRE(target.x == 1.);
        REQUIRE(hull.intersectRay(Ray(Vector3(1, 0, -5), Vector3(0, 0, 1)), target) == &target);
        REQUIRE(target.x == 1.);
        REQUIRE(target.z == -1.);
        REQUIRE(hull.intersectsRay(Ray(Vector3(0, 0, -5), Vector3(0, 0, 1))));
    }

    SECTION("a miss returns nullptr and leaves the target alone") {
        Vector3 target(7., 7., 7.);
        REQUIRE(hull.intersectRay(Ray(Vector3(0, 0, -5), Vector3(0, 0, -1)), target) == nullptr);
        REQUIRE_FALSE(hull.intersectsRay(Ray(Vector3(0, 0, -5), Vector3(0, 0, -1))));
        REQUIRE(target.x == 7.);
        REQUIRE(target.y == 7.);
        REQUIRE(target.z == 7.);
    }

    SECTION("a hull without faces still evaluates the infinite tFar") {
        // No face is tested, so tNear stays -Infinity and the pinned code evaluates
        // ray.at( Infinity ): the reference run yields ( NaN, NaN, Infinity ) for this ray.
        const ConvexHull empty;
        Vector3 target;
        REQUIRE(empty.intersectRay(Ray(Vector3(0, 0, -5), Vector3(0, 0, 1)), target) != nullptr);
        REQUIRE(std::isnan(target.x));
        REQUIRE(std::isnan(target.y));
        REQUIRE(target.z == std::numeric_limits<double>::infinity());
        REQUIRE(empty.intersectsRay(Ray(Vector3(0, 0, -5), Vector3(0, 0, 1))));
    }
}

TEST_CASE("ConvexHull setFromPoints needs at least four points", "[convexhull]") {
    ConvexHull hull;

    SECTION("zero to three points leave the hull untouched") {
        const std::vector<std::vector<Vector3>> clouds = {
            {},
            {Vector3(0.5, -0.25, 3)},
            {Vector3(0, 0, 0), Vector3(1, 1, 1)},
            {Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0)}};
        for (const std::vector<Vector3>& cloud : clouds) {
            REQUIRE(&hull.setFromPoints(cloud) == &hull);
            REQUIRE(hull.tolerance == -1.);
            REQUIRE(hull.faces.empty());
            REQUIRE(hull.vertices.empty());
            // With no faces every point is "inside", like in the pinned module.
            REQUIRE(hull.containsPoint(Vector3(5, 5, 5)));
        }
    }

    SECTION("a planar grid still produces the four degenerate pinned faces") {
        std::vector<Vector3> grid;
        for (int x = 0; x < 4; x++)
            for (int y = 0; y < 4; y++) grid.push_back(Vector3(x - 1.5, y - 1.5, 0));
        hull.setFromPoints(grid);
        REQUIRE(hull.vertices.size() == 16);
        REQUIRE(hull.faces.size() == 4);
        REQUIRE(hull.tolerance == cubeTolerance());
        // v3 is the first vertex that does not lie on the plane through v0, v1 and v2, so the
        // pinned module takes the "normal points inside" branch of _computeInitialHull.
        REQUIRE(faceVertexIndices(hull, hull.faces[0]) == std::vector<int>{0, 3, 12});
        REQUIRE(faceVertexIndices(hull, hull.faces[1]) == std::vector<int>{1, 0, 12});
        REQUIRE(faceVertexIndices(hull, hull.faces[2]) == std::vector<int>{1, 12, 3});
        REQUIRE(faceVertexIndices(hull, hull.faces[3]) == std::vector<int>{1, 3, 0});
    }

    SECTION("collinear and identical points throw the pinned TypeError") {
        const std::vector<std::vector<Vector3>> clouds = {
            {Vector3(0, 0, 0), Vector3(1, 1, 1), Vector3(2, 2, 2), Vector3(-1, -1, -1),
             Vector3(0.5, 0.5, 0.5)},
            {Vector3(0.5, 0.5, 0.5), Vector3(0.5, 0.5, 0.5), Vector3(0.5, 0.5, 0.5),
             Vector3(0.5, 0.5, 0.5)}};
        for (const std::vector<Vector3>& cloud : clouds) {
            ConvexHull degenerate;
            REQUIRE_THROWS_AS(degenerate.setFromPoints(cloud), JSTypeError);
            REQUIRE_THROWS_WITH(degenerate.setFromPoints(cloud),
                                "Cannot read properties of undefined (reading 'point')");
        }
    }
}

TEST_CASE("ConvexHull setFromPoints replaces the previous hull", "[convexhull]") {
    ConvexHull hull;
    hull.setFromPoints(cubeCorners());
    REQUIRE(hull.faces.size() == 12);

    const std::vector<Vector3> tetrahedron = {Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 1, 0),
                                              Vector3(0, 0, 1)};
    hull.setFromPoints(tetrahedron);
    REQUIRE(hull.vertices.size() == 4);
    REQUIRE(hull.faces.size() == 4);
    REQUIRE(hull.tolerance == 3 * std::numeric_limits<double>::epsilon() * (1. + 1. + 1.));
    REQUIRE(hull.containsPoint(Vector3(0.25, 0.25, 0.25)));
    REQUIRE_FALSE(hull.containsPoint(Vector3(1, 1, 1)));
}

TEST_CASE("ConvexHull setFromObject uses the world matrices", "[convexhull]") {
    SECTION("the parent world matrix is applied to the child geometry") {
        auto root = std::make_shared<Object3D>();
        root->position.set(1, 0, 0);
        auto mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(2, 2, 2));
        root->add(mesh);

        ConvexHull hull;
        REQUIRE(&hull.setFromObject(*root) == &hull);
        // `vertices` holds every candidate point that was read from the geometry, so the
        // 24 position entries of BoxGeometry( 2, 2, 2 ) are all listed; only 8 of them end up
        // on the hull.
        REQUIRE(hull.vertices.size() == 24);
        REQUIRE(hull.faces.size() == 12);
        REQUIRE(hull.containsPoint(Vector3(1, 0, 0)));
        REQUIRE(hull.containsPoint(Vector3(2, 1, 1)));
        REQUIRE_FALSE(hull.containsPoint(Vector3(-1, 0, 0)));
        // The points span [ 0, 2 ] x [ -1, 1 ] x [ -1, 1 ].
        REQUIRE(hull.tolerance == 3 * std::numeric_limits<double>::epsilon() * (2. + 1. + 1.));
    }

    SECTION("sibling meshes and their local transforms are combined") {
        auto root = std::make_shared<Object3D>();
        auto left = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(2, 2, 2));
        left->position.set(-5, 0, 0);
        auto right = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(2, 2, 2));
        right->position.set(5, 0, 0);
        root->add(left, right);

        ConvexHull hull;
        hull.setFromObject(*root);
        REQUIRE(hull.vertices.size() == 48);
        REQUIRE(hull.faces.size() == 12);
        REQUIRE(hull.containsPoint(Vector3(6, 1, 1)));
        REQUIRE(hull.containsPoint(Vector3(-6, -1, -1)));
        REQUIRE_FALSE(hull.containsPoint(Vector3(6.5, 0, 0)));
        REQUIRE(hull.tolerance == 3 * std::numeric_limits<double>::epsilon() * (6. + 1. + 1.));
    }

    SECTION("the scale of a node is part of the world matrix") {
        auto mesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(1, 1, 1));
        mesh->scale.set(2, 1, 1);

        ConvexHull hull;
        hull.setFromObject(*mesh);
        REQUIRE(hull.vertices.size() == 24);
        REQUIRE(hull.faces.size() == 12);
        REQUIRE(hull.containsPoint(Vector3(1, 0.5, 0.5)));
        REQUIRE_FALSE(hull.containsPoint(Vector3(1.5, 0, 0)));
    }

    SECTION("nodes without a position attribute contribute no points") {
        auto root = std::make_shared<Object3D>();
        root->add(std::make_shared<Object3D>());
        root->add(std::make_shared<Mesh>(std::make_shared<BufferGeometry>()));
        root->add(std::make_shared<Mesh>(std::make_shared<BoxGeometry>(2, 2, 2)));

        ConvexHull hull;
        hull.setFromObject(*root);
        // Only the box contributes, so the hull of the pinned module is the unit cube again.
        REQUIRE(hull.vertices.size() == 24);
        REQUIRE(hull.faces.size() == 12);
        REQUIRE(hull.tolerance == cubeTolerance());
    }
}

TEST_CASE("ConvexHull exposes the pinned intermediates", "[convexhull]") {
    // The arena of a hull owns only the nodes it created, so a caller that drives the private
    // steps itself keeps its own vertex nodes alive.
    std::vector<std::unique_ptr<VertexNode>> nodes;
    ConvexHull hull;
    for (const Vector3& point : cubeCorners()) {
        nodes.push_back(std::make_unique<VertexNode>(point));
        hull.vertices.push_back(nodes.back().get());
    }

    SECTION("_computeExtremes picks the pinned min/max vertices") {
        const ConvexHull::Extremes extremes = hull._computeExtremes();
        REQUIRE(vertexIndex(hull, extremes.min[0]) == 0);
        REQUIRE(vertexIndex(hull, extremes.min[1]) == 0);
        REQUIRE(vertexIndex(hull, extremes.min[2]) == 0);
        REQUIRE(vertexIndex(hull, extremes.max[0]) == 4);
        REQUIRE(vertexIndex(hull, extremes.max[1]) == 2);
        REQUIRE(vertexIndex(hull, extremes.max[2]) == 1);
        REQUIRE(hull.tolerance == cubeTolerance());
    }

    SECTION("_computeInitialHull builds the pinned tetrahedron") {
        hull._computeExtremes();
        REQUIRE(&hull._computeInitialHull() == &hull);
        REQUIRE(hull.faces.size() == 4);
        REQUIRE(hull.newFaces.empty());
        REQUIRE_FALSE(hull.assigned.isEmpty());
        REQUIRE(hull.unassigned.isEmpty());
        REQUIRE(faceVertexIndices(hull, hull.faces[0]) == std::vector<int>{0, 3, 4});
        REQUIRE(faceVertexIndices(hull, hull.faces[1]) == std::vector<int>{1, 0, 4});
        REQUIRE(faceVertexIndices(hull, hull.faces[2]) == std::vector<int>{1, 4, 3});
        REQUIRE(faceVertexIndices(hull, hull.faces[3]) == std::vector<int>{1, 3, 0});
        // The assigned list holds the four remaining vertices in the pinned order.
        REQUIRE(vertexChain(hull, hull.assigned.head) == std::vector<int>{6, 2, 7, 5});
        REQUIRE(vertexIndex(hull, hull.assigned.first()) == 6);
        REQUIRE(vertexIndex(hull, hull.assigned.last()) == 5);
        // _nextVertexToAdd() walks the assigned list through 'next' from face.outside.
        REQUIRE(vertexIndex(hull, hull._nextVertexToAdd()) == 6);
        for (const VertexNode* vertex : hull.vertices)
            if (vertex->face != nullptr) REQUIRE(faceIndex(hull, vertex->face) >= 0);
    }
}

TEST_CASE("Face, HalfEdge and VertexList primitives", "[convexhull]") {
    SECTION("Face::create and compute use the pinned triangle formulas") {
        ConvexHullArena arena;
        VertexNode* a = arena.createVertexNode(Vector3(0, 0, 0));
        VertexNode* b = arena.createVertexNode(Vector3(1, 0, 0));
        VertexNode* c = arena.createVertexNode(Vector3(0, 1, 0));
        Face* face = Face::create(arena, a, b, c);

        REQUIRE(face->mark == Face::Visible);
        REQUIRE(face->outside == nullptr);
        REQUIRE(face->edge->vertex == a);
        REQUIRE(face->getEdge(0) == face->edge);
        REQUIRE(face->getEdge(1) == face->edge->next);
        REQUIRE(face->getEdge(2) == face->edge->next->next);
        // getEdge() wraps in both directions.
        REQUIRE(face->getEdge(3) == face->edge);
        REQUIRE(face->getEdge(-1) == face->edge->prev);
        REQUIRE(face->getEdge(-2) == face->edge->prev->prev);
        REQUIRE(face->getEdge(-3) == face->edge);

        REQUIRE(face->normal.x == 0.);
        REQUIRE(face->normal.y == 0.);
        REQUIRE(face->normal.z == 1.);
        REQUIRE(face->area == 0.5);
        REQUIRE(face->constant == 0.);
        REQUIRE(face->midpoint.x == 1. / 3.);
        REQUIRE(face->midpoint.y == 1. / 3.);
        REQUIRE(face->midpoint.z == 0.);
        REQUIRE(face->distanceToPoint(Vector3(0, 0, 5)) == 5.);
        REQUIRE(face->distanceToPoint(Vector3(0, 0, -2)) == -2.);
        REQUIRE(&face->compute() == face);
    }

    SECTION("HalfEdge head, tail, lengths and twins") {
        ConvexHullArena arena;
        VertexNode* a = arena.createVertexNode(Vector3(0, 0, 0));
        VertexNode* b = arena.createVertexNode(Vector3(1, 0, 0));
        VertexNode* c = arena.createVertexNode(Vector3(0, 1, 0));
        Face* face = Face::create(arena, a, b, c);

        // e0 ends at a, so its tail is the vertex of the previous edge, c.
        HalfEdge* e0 = face->edge;
        REQUIRE(e0->head() == a);
        REQUIRE(e0->tail() == c);
        REQUIRE(e0->length() == 1.);
        REQUIRE(e0->lengthSquared() == 1.);
        REQUIRE(e0->length() == e0->tail()->point.distanceTo(e0->head()->point));
        REQUIRE(e0->prev->head() == c);
        REQUIRE(e0->prev->tail() == b);
        REQUIRE(e0->prev->lengthSquared() == 2.);
        REQUIRE(e0->next->head() == b);
        REQUIRE(e0->next->tail() == a);

        // A standalone edge has no previous edge: the pinned module returns -1.
        HalfEdge orphan(a, face);
        REQUIRE(orphan.head() == a);
        REQUIRE(orphan.tail() == nullptr);
        REQUIRE(orphan.length() == -1.);
        REQUIRE(orphan.lengthSquared() == -1.);

        HalfEdge* other = arena.createHalfEdge(b, face);
        REQUIRE(e0->setTwin(other) == e0);
        REQUIRE(e0->twin == other);
        REQUIRE(other->twin == e0);
    }

    SECTION("VertexList keeps the pinned doubly linked list") {
        ConvexHullArena arena;
        std::vector<VertexNode*> nodes;
        for (int i = 0; i < 5; i++) nodes.push_back(arena.createVertexNode(Vector3(i, 0, 0)));

        VertexList list;
        REQUIRE(list.isEmpty());
        REQUIRE(list.first() == nullptr);
        REQUIRE(list.last() == nullptr);

        REQUIRE(&list.append(nodes[0]) == &list);
        REQUIRE(list.first() == nodes[0]);
        REQUIRE(list.last() == nodes[0]);
        REQUIRE(nodes[0]->prev == nullptr);
        REQUIRE(nodes[0]->next == nullptr);

        list.append(nodes[1]);
        REQUIRE(list.last() == nodes[1]);
        REQUIRE(nodes[0]->next == nodes[1]);
        REQUIRE(nodes[1]->prev == nodes[0]);
        REQUIRE(nodes[1]->next == nullptr);

        // insertBefore/insertAfter place a vertex relative to a target.
        list.insertBefore(nodes[0], nodes[2]);
        REQUIRE(list.first() == nodes[2]);
        REQUIRE(nodes[2]->next == nodes[0]);
        REQUIRE(nodes[0]->prev == nodes[2]);
        list.insertAfter(nodes[1], nodes[3]);
        REQUIRE(list.last() == nodes[3]);
        REQUIRE(nodes[1]->next == nodes[3]);
        REQUIRE(nodes[3]->prev == nodes[1]);

        // remove() fixes the head/tail references of the list.
        list.remove(nodes[2]);
        REQUIRE(list.first() == nodes[0]);
        list.remove(nodes[3]);
        REQUIRE(list.last() == nodes[1]);

        // appendChain() links a whole chain and moves the tail to its end.
        nodes[2]->next = nodes[3];
        nodes[3]->prev = nodes[2];
        nodes[3]->next = nullptr;
        list.appendChain(nodes[2]);
        REQUIRE(list.last() == nodes[3]);
        REQUIRE(nodes[1]->next == nodes[2]);
        REQUIRE(vertexChainOf(list) ==
                std::vector<VertexNode*>({nodes[0], nodes[1], nodes[2], nodes[3]}));

        // removeSubList() removes the inclusive range [ a, b ].
        list.removeSubList(nodes[1], nodes[2]);
        REQUIRE(vertexChainOf(list) == std::vector<VertexNode*>({nodes[0], nodes[3]}));
        list.removeSubList(nodes[0], nodes[3]);
        REQUIRE(list.isEmpty());
        REQUIRE(list.first() == nullptr);
        REQUIRE(list.last() == nullptr);

        list.append(nodes[4]);
        REQUIRE(&list.clear() == &list);
        REQUIRE(list.isEmpty());
        REQUIRE(list.first() == nullptr);
        REQUIRE(list.last() == nullptr);
    }
}
