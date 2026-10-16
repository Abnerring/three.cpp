#include "math/ConvexHull.h"
#include "core/BufferGeometry.h"
#include "core/JSValue.h"
#include "core/Object3D.h"
#include "core/VertexAttribute.h"
#include "math/Line3.h"
#include "math/MathUtils.h"
#include "math/Plane.h"
#include "math/Ray.h"
#include "math/Triangle.h"
#include "objects/Line.h"
#include "objects/Mesh.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include <cmath>
#include <limits>

namespace {

// The module scope variables of the pinned source, one set per owning thread. `_v1` is the
// target of intersectsRay(), `_line3`, `_plane`, `_closestPoint` and `_triangle` are shared by
// the hull construction and by Face::compute() exactly like upstream.
thread_local Vector3 _v1;
thread_local Line3 _line3;
thread_local Plane _plane;
thread_local Vector3 _closestPoint;
thread_local Triangle _triangle;

// The native counterpart of reading `node.geometry` while traversing: only objects that own a
// geometry can contribute points (`undefined` nodes are skipped by the source).
BufferGeometry* geometryOf(Object3D& node) {
    if (auto* mesh = dynamic_cast<Mesh*>(&node)) return mesh->geometry.get();
    if (auto* line = dynamic_cast<Line*>(&node)) return line->geometry.get();
    if (auto* points = dynamic_cast<Points*>(&node)) return points->geometry.get();
    if (auto* sprite = dynamic_cast<Sprite*>(&node)) return sprite->geometry.get();
    return nullptr;
}

} // namespace

VertexNode::VertexNode(const Vector3& p) : point(p) {}

HalfEdge::HalfEdge(VertexNode* v, Face* f) : vertex(v), face(f) {}

VertexNode* HalfEdge::head() const { return this->vertex; }

VertexNode* HalfEdge::tail() const { return this->prev ? this->prev->vertex : nullptr; }

double HalfEdge::length() const {
    VertexNode* head = this->head();
    VertexNode* tail = this->tail();

    if (tail != nullptr) {
        return tail->point.distanceTo(head->point);
    }

    return -1;
}

double HalfEdge::lengthSquared() const {
    VertexNode* head = this->head();
    VertexNode* tail = this->tail();

    if (tail != nullptr) {
        return tail->point.distanceToSquared(head->point);
    }

    return -1;
}

HalfEdge* HalfEdge::setTwin(HalfEdge* edge) {
    this->twin = edge;
    edge->twin = this;

    return this;
}

VertexNode* VertexList::first() const { return this->head; }

VertexNode* VertexList::last() const { return this->tail; }

VertexList& VertexList::clear() {
    this->head = this->tail = nullptr;

    return *this;
}

VertexList& VertexList::insertBefore(VertexNode* target, VertexNode* vertex) {
    vertex->prev = target->prev;
    vertex->next = target;

    if (vertex->prev == nullptr) {
        this->head = vertex;
    } else {
        vertex->prev->next = vertex;
    }

    target->prev = vertex;

    return *this;
}

VertexList& VertexList::insertAfter(VertexNode* target, VertexNode* vertex) {
    vertex->prev = target;
    vertex->next = target->next;

    if (vertex->next == nullptr) {
        this->tail = vertex;
    } else {
        vertex->next->prev = vertex;
    }

    target->next = vertex;

    return *this;
}

VertexList& VertexList::append(VertexNode* vertex) {
    if (this->head == nullptr) {
        this->head = vertex;
    } else {
        this->tail->next = vertex;
    }

    vertex->prev = this->tail;
    vertex->next = nullptr; // the tail has no subsequent vertex

    this->tail = vertex;

    return *this;
}

VertexList& VertexList::appendChain(VertexNode* vertex) {
    if (this->head == nullptr) {
        this->head = vertex;
    } else {
        this->tail->next = vertex;
    }

    vertex->prev = this->tail;

    // ensure that the 'tail' reference points to the last vertex of the chain

    while (vertex->next != nullptr) {
        vertex = vertex->next;
    }

    this->tail = vertex;

    return *this;
}

VertexList& VertexList::remove(VertexNode* vertex) {
    if (vertex->prev == nullptr) {
        this->head = vertex->next;
    } else {
        vertex->prev->next = vertex->next;
    }

    if (vertex->next == nullptr) {
        this->tail = vertex->prev;
    } else {
        vertex->next->prev = vertex->prev;
    }

    return *this;
}

VertexList& VertexList::removeSubList(VertexNode* a, VertexNode* b) {
    if (a->prev == nullptr) {
        this->head = b->next;
    } else {
        a->prev->next = b->next;
    }

    if (b->next == nullptr) {
        this->tail = a->prev;
    } else {
        b->next->prev = a->prev;
    }

    return *this;
}

bool VertexList::isEmpty() const { return this->head == nullptr; }

Face* Face::create(ConvexHullArena& arena, VertexNode* a, VertexNode* b, VertexNode* c) {
    Face* face = arena.createFace();

    HalfEdge* e0 = arena.createHalfEdge(a, face);
    HalfEdge* e1 = arena.createHalfEdge(b, face);
    HalfEdge* e2 = arena.createHalfEdge(c, face);

    // join edges

    e0->next = e2->prev = e1;
    e1->next = e0->prev = e2;
    e2->next = e1->prev = e0;

    // main half edge reference

    face->edge = e0;

    return &face->compute();
}

HalfEdge* Face::getEdge(int i) const {
    // `current` instead of the upstream local `edge`, which would hide the member.
    HalfEdge* current = this->edge;

    while (i > 0) {
        current = current->next;
        i--;
    }

    while (i < 0) {
        current = current->prev;
        i++;
    }

    return current;
}

Face& Face::compute() {
    VertexNode* a = this->edge->tail();
    VertexNode* b = this->edge->head();
    VertexNode* c = this->edge->next->head();

    _triangle.set(a->point, b->point, c->point);

    _triangle.getNormal(this->normal);
    _triangle.getMidpoint(this->midpoint);
    this->area = _triangle.getArea();

    this->constant = this->normal.dot(this->midpoint);

    return *this;
}

double Face::distanceToPoint(const Vector3& point) const {
    return this->normal.dot(point) - this->constant;
}

Face* ConvexHullArena::createFace() {
    faces.push_back(std::make_unique<Face>());
    return faces.back().get();
}

HalfEdge* ConvexHullArena::createHalfEdge(VertexNode* vertex, Face* face) {
    halfEdges.push_back(std::make_unique<HalfEdge>(vertex, face));
    return halfEdges.back().get();
}

VertexNode* ConvexHullArena::createVertexNode(const Vector3& point) {
    vertexNodes.push_back(std::make_unique<VertexNode>(point));
    return vertexNodes.back().get();
}

// The constructor defaults of the source: `tolerance = -1`, empty arrays and two empty vertex
// lists (all of them are member initializers).
ConvexHull::ConvexHull() = default;

ConvexHull& ConvexHull::setFromPoints(const std::vector<Vector3>& points) {
    // The algorithm needs at least four points.

    if (points.size() >= 4) {
        this->makeEmpty();

        for (std::size_t i = 0, l = points.size(); i < l; i++) {
            this->vertices.push_back(this->arena_.createVertexNode(points[i]));
        }

        this->_compute();
    }

    return *this;
}

ConvexHull& ConvexHull::setFromObject(Object3D& object) {
    std::vector<Vector3> points;

    object.updateMatrixWorld(true);

    object.traverse([&points](Object3D& node) {
        BufferGeometry* geometry = geometryOf(node);

        if (geometry != nullptr) {
            const std::shared_ptr<VertexAttribute> attribute = geometry->getAttribute("position");

            if (attribute != nullptr) {
                for (double i = 0, l = attribute->getCount(); i < l; i++) {
                    Vector3 point;

                    point.fromBufferAttribute(*attribute, i).applyMatrix4(node.matrixWorld);

                    points.push_back(point);
                }
            }
        }
    });

    return this->setFromPoints(points);
}

bool ConvexHull::containsPoint(const Vector3& point) const {
    // `faceList`/`hullVertices` replace the upstream locals `faces`/`vertices`, which would
    // hide the members of the same name in C++.
    const std::vector<Face*>& faceList = this->faces;

    for (std::size_t i = 0, l = faceList.size(); i < l; i++) {
        Face* face = faceList[i];

        // compute signed distance and check on what half space the point lies

        if (face->distanceToPoint(point) > this->tolerance) return false;
    }

    return true;
}

Vector3* ConvexHull::intersectRay(const Ray& ray, Vector3& target) const {
    // based on "Fast Ray-Convex Polyhedron Intersection" by Eric Haines, GRAPHICS GEMS II

    const std::vector<Face*>& faceList = this->faces;

    double tNear = -std::numeric_limits<double>::infinity();
    double tFar = std::numeric_limits<double>::infinity();

    for (std::size_t i = 0, l = faceList.size(); i < l; i++) {
        Face* face = faceList[i];

        // interpret faces as planes for the further computation

        const double vN = face->distanceToPoint(ray.origin);
        const double vD = face->normal.dot(ray.direction);

        // if the origin is on the positive side of a plane (so the plane can "see" the origin)
        // and the ray is turned away or parallel to the plane, there is no intersection

        if (vN > 0 && vD >= 0) return nullptr;

        // compute the distance from the ray's origin to the intersection with the plane

        const double t = (vD != 0) ? (-vN / vD) : 0;

        // only proceed if the distance is positive. a negative distance means the intersection
        // point lies "behind" the origin

        if (t <= 0) continue;

        // now categorized plane as front-facing or back-facing

        if (vD > 0) {
            // plane faces away from the ray, so this plane is a back-face

            // Math.min/Math.max keep NaN and the sign of zero, std::min/std::max do not.
            tFar = MathUtils::min(t, tFar);
        } else {
            // front-face

            tNear = MathUtils::max(t, tNear);
        }

        if (tNear > tFar) {
            // if tNear ever is greater than tFar, the ray must miss the convex hull

            return nullptr;
        }
    }

    // evaluate intersection point

    // always try tNear first since its the closer intersection point

    if (tNear != -std::numeric_limits<double>::infinity()) {
        ray.at(tNear, target);
    } else {
        ray.at(tFar, target);
    }

    return &target;
}

bool ConvexHull::intersectsRay(const Ray& ray) const {
    return this->intersectRay(ray, _v1) != nullptr;
}

ConvexHull& ConvexHull::makeEmpty() {
    this->faces.clear();
    this->vertices.clear();

    return *this;
}

ConvexHull& ConvexHull::_addVertexToFace(VertexNode* vertex, Face* face) {
    vertex->face = face;

    if (face->outside == nullptr) {
        this->assigned.append(vertex);
    } else {
        this->assigned.insertBefore(face->outside, vertex);
    }

    face->outside = vertex;

    return *this;
}

ConvexHull& ConvexHull::_removeVertexFromFace(VertexNode* vertex, Face* face) {
    if (vertex == face->outside) {
        // fix face.outside link

        if (vertex->next != nullptr && vertex->next->face == face) {
            // face has at least 2 outside vertices, move the 'outside' reference

            face->outside = vertex->next;
        } else {
            // vertex was the only outside vertex that face had

            face->outside = nullptr;
        }
    }

    this->assigned.remove(vertex);

    return *this;
}

VertexNode* ConvexHull::_removeAllVerticesFromFace(Face* face) {
    if (face->outside != nullptr) {
        // reference to the first and last vertex of this face

        VertexNode* start = face->outside;
        VertexNode* end = face->outside;

        while (end->next != nullptr && end->next->face == face) {
            end = end->next;
        }

        this->assigned.removeSubList(start, end);

        // fix references

        start->prev = nullptr;
        end->next = nullptr;
        face->outside = nullptr;

        return start;
    }

    return nullptr;
}

ConvexHull& ConvexHull::_deleteFaceVertices(Face* face, Face* absorbingFace) {
    VertexNode* faceVertices = this->_removeAllVerticesFromFace(face);

    if (faceVertices != nullptr) {
        if (absorbingFace == nullptr) {
            // mark the vertices to be reassigned to some other face

            this->unassigned.appendChain(faceVertices);
        } else {
            // if there's an absorbing face try to assign as many vertices as possible to it

            VertexNode* vertex = faceVertices;

            do {
                // we need to buffer the subsequent vertex at this point because the 'vertex.next'
                // reference will be changed by upcoming method calls

                VertexNode* nextVertex = vertex->next;

                const double distance = absorbingFace->distanceToPoint(vertex->point);

                // check if 'vertex' is able to see 'absorbingFace'

                if (distance > this->tolerance) {
                    this->_addVertexToFace(vertex, absorbingFace);
                } else {
                    this->unassigned.append(vertex);
                }

                // now assign next vertex

                vertex = nextVertex;
            } while (vertex != nullptr);
        }
    }

    return *this;
}

ConvexHull& ConvexHull::_resolveUnassignedPoints(const std::vector<Face*>& newFaceList) {
    if (this->unassigned.isEmpty() == false) {
        VertexNode* vertex = this->unassigned.first();

        do {
            // buffer 'next' reference, see ._deleteFaceVertices()

            VertexNode* nextVertex = vertex->next;

            double maxDistance = this->tolerance;

            Face* maxFace = nullptr;

            for (std::size_t i = 0; i < newFaceList.size(); i++) {
                Face* face = newFaceList[i];

                if (face->mark == Face::Visible) {
                    const double distance = face->distanceToPoint(vertex->point);

                    if (distance > maxDistance) {
                        maxDistance = distance;
                        maxFace = face;
                    }

                    if (maxDistance > 1000 * this->tolerance) break;
                }
            }

            // 'maxFace' can be null e.g. if there are identical vertices

            if (maxFace != nullptr) {
                this->_addVertexToFace(vertex, maxFace);
            }

            vertex = nextVertex;
        } while (vertex != nullptr);
    }

    return *this;
}

ConvexHull::Extremes ConvexHull::_computeExtremes() {
    if (this->vertices.empty()) {
        // the source reads `this.vertices[ 0 ].point` of an empty hull here
        throw JSTypeError("Cannot read properties of undefined (reading 'point')");
    }

    Vector3 min;
    Vector3 max;

    VertexNode* minVertices[3];
    VertexNode* maxVertices[3];

    // initially assume that the first vertex is the min/max

    for (int i = 0; i < 3; i++) {
        minVertices[i] = maxVertices[i] = this->vertices[0];
    }

    min.copy(this->vertices[0]->point);
    max.copy(this->vertices[0]->point);

    // compute the min/max vertex on all six directions

    for (std::size_t i = 0, l = this->vertices.size(); i < l; i++) {
        VertexNode* vertex = this->vertices[i];
        const Vector3& point = vertex->point;

        // update the min coordinates

        for (std::size_t j = 0; j < 3; j++) {
            if (point.getComponent(j) < min.getComponent(j)) {
                min.setComponent(j, point.getComponent(j));
                minVertices[j] = vertex;
            }
        }

        // update the max coordinates

        for (std::size_t j = 0; j < 3; j++) {
            if (point.getComponent(j) > max.getComponent(j)) {
                max.setComponent(j, point.getComponent(j));
                maxVertices[j] = vertex;
            }
        }
    }

    // use min/max vectors to compute an optimal epsilon

    // Math.max is used instead of std::max: it propagates NaN and keeps the sign of zero.
    this->tolerance = 3 * std::numeric_limits<double>::epsilon() *
                      (MathUtils::max(std::fabs(min.x), std::fabs(max.x)) +
                       MathUtils::max(std::fabs(min.y), std::fabs(max.y)) +
                       MathUtils::max(std::fabs(min.z), std::fabs(max.z)));

    return Extremes{{minVertices[0], minVertices[1], minVertices[2]},
                    {maxVertices[0], maxVertices[1], maxVertices[2]}};
}

ConvexHull& ConvexHull::_computeInitialHull() {
    // `hullVertices` and `initialFaces` replace the upstream locals `vertices` and `faces`,
    // which would hide the members of the same name in C++.
    const std::vector<VertexNode*>& hullVertices = this->vertices;
    const Extremes extremes = this->_computeExtremes();
    VertexNode* const* min = extremes.min;
    VertexNode* const* max = extremes.max;

    // 1. Find the two vertices 'v0' and 'v1' with the greatest 1d separation
    // (max.x - min.x)
    // (max.y - min.y)
    // (max.z - min.z)

    double maxDistance = 0;
    int index = 0;

    for (int i = 0; i < 3; i++) {
        const double distance =
            max[i]->point.getComponent(std::size_t(i)) - min[i]->point.getComponent(std::size_t(i));

        if (distance > maxDistance) {
            maxDistance = distance;
            index = i;
        }
    }

    VertexNode* v0 = min[index];
    VertexNode* v1 = max[index];
    VertexNode* v2 = nullptr;
    VertexNode* v3 = nullptr;

    // 2. The next vertex 'v2' is the one farthest to the line formed by 'v0' and 'v1'

    maxDistance = 0;
    _line3.set(v0->point, v1->point);

    for (std::size_t i = 0, l = hullVertices.size(); i < l; i++) {
        VertexNode* vertex = hullVertices[i];

        if (vertex != v0 && vertex != v1) {
            _line3.closestPointToPoint(vertex->point, true, _closestPoint);

            const double distance = _closestPoint.distanceToSquared(vertex->point);

            if (distance > maxDistance) {
                maxDistance = distance;
                v2 = vertex;
            }
        }
    }

    if (v2 == nullptr) {
        // the source reads `v2.point` of an undefined vertex here
        throw JSTypeError("Cannot read properties of undefined (reading 'point')");
    }

    // 3. The next vertex 'v3' is the one farthest to the plane 'v0', 'v1', 'v2'

    maxDistance = -1;
    _plane.setFromCoplanarPoints(v0->point, v1->point, v2->point);

    for (std::size_t i = 0, l = hullVertices.size(); i < l; i++) {
        VertexNode* vertex = hullVertices[i];

        if (vertex != v0 && vertex != v1 && vertex != v2) {
            const double distance = std::fabs(_plane.distanceToPoint(vertex->point));

            if (distance > maxDistance) {
                maxDistance = distance;
                v3 = vertex;
            }
        }
    }

    if (v3 == nullptr) {
        // the source reads `v3.point` of an undefined vertex here
        throw JSTypeError("Cannot read properties of undefined (reading 'point')");
    }

    std::vector<Face*> initialFaces;

    if (_plane.distanceToPoint(v3->point) < 0) {
        // the face is not able to see the point so 'plane.normal' is pointing outside the
        // tetrahedron

        initialFaces.push_back(Face::create(this->arena_, v0, v1, v2));
        initialFaces.push_back(Face::create(this->arena_, v3, v1, v0));
        initialFaces.push_back(Face::create(this->arena_, v3, v2, v1));
        initialFaces.push_back(Face::create(this->arena_, v3, v0, v2));

        // set the twin edge

        for (int i = 0; i < 3; i++) {
            const int j = (i + 1) % 3;

            // join face[ i ] i > 0, with the first face

            initialFaces[std::size_t(i) + 1]->getEdge(2)->setTwin(initialFaces[0]->getEdge(j));

            // join face[ i ] with face[ i + 1 ], 1 <= i <= 3

            initialFaces[std::size_t(i) + 1]->getEdge(1)->setTwin(
                initialFaces[std::size_t(j) + 1]->getEdge(0));
        }

    } else {
        // the face is able to see the point so 'plane.normal' is pointing inside the tetrahedron

        initialFaces.push_back(Face::create(this->arena_, v0, v2, v1));
        initialFaces.push_back(Face::create(this->arena_, v3, v0, v1));
        initialFaces.push_back(Face::create(this->arena_, v3, v1, v2));
        initialFaces.push_back(Face::create(this->arena_, v3, v2, v0));

        // set the twin edge

        for (int i = 0; i < 3; i++) {
            const int j = (i + 1) % 3;

            // join face[ i ] i > 0, with the first face

            initialFaces[std::size_t(i) + 1]->getEdge(2)->setTwin(
                initialFaces[0]->getEdge((3 - i) % 3));

            // join face[ i ] with face[ i + 1 ]

            initialFaces[std::size_t(i) + 1]->getEdge(0)->setTwin(
                initialFaces[std::size_t(j) + 1]->getEdge(1));
        }
    }

    // the initial hull is the tetrahedron

    for (int i = 0; i < 4; i++) {
        this->faces.push_back(initialFaces[std::size_t(i)]);
    }

    // initial assignment of vertices to the faces of the tetrahedron

    for (std::size_t i = 0, l = hullVertices.size(); i < l; i++) {
        VertexNode* vertex = hullVertices[i];

        if (vertex != v0 && vertex != v1 && vertex != v2 && vertex != v3) {
            maxDistance = this->tolerance;
            Face* maxFace = nullptr;

            for (int j = 0; j < 4; j++) {
                const double distance = this->faces[std::size_t(j)]->distanceToPoint(vertex->point);

                if (distance > maxDistance) {
                    maxDistance = distance;
                    maxFace = this->faces[std::size_t(j)];
                }
            }

            if (maxFace != nullptr) {
                this->_addVertexToFace(vertex, maxFace);
            }
        }
    }

    return *this;
}

ConvexHull& ConvexHull::_reindexFaces() {
    std::vector<Face*> activeFaces;

    for (std::size_t i = 0; i < this->faces.size(); i++) {
        Face* face = this->faces[i];

        if (face->mark == Face::Visible) {
            activeFaces.push_back(face);
        }
    }

    this->faces = activeFaces;

    return *this;
}

VertexNode* ConvexHull::_nextVertexToAdd() {
    // if the 'assigned' list of vertices is empty, no vertices are left. return with
    // 'undefined'

    if (this->assigned.isEmpty() == false) {
        VertexNode* eyeVertex = nullptr;
        double maxDistance = 0;

        // grab the first available face and start with the first visible vertex of that face

        Face* eyeFace = this->assigned.first()->face;
        VertexNode* vertex = eyeFace->outside;

        // now calculate the farthest vertex that face can see

        do {
            const double distance = eyeFace->distanceToPoint(vertex->point);

            if (distance > maxDistance) {
                maxDistance = distance;
                eyeVertex = vertex;
            }

            vertex = vertex->next;

        } while (vertex != nullptr && vertex->face == eyeFace);

        return eyeVertex;
    }

    return nullptr;
}

ConvexHull& ConvexHull::_computeHorizon(const Vector3& eyePoint, HalfEdge* crossEdge, Face* face,
                                        std::vector<HalfEdge*>& horizon) {
    // moves face's vertices to the 'unassigned' vertex list

    this->_deleteFaceVertices(face);

    face->mark = Face::Deleted;

    HalfEdge* edge;

    if (crossEdge == nullptr) {
        edge = crossEdge = face->getEdge(0);
    } else {
        // start from the next edge since 'crossEdge' was already analyzed
        // (actually 'crossEdge.twin' was the edge who called this method recursively)

        edge = crossEdge->next;
    }

    do {
        HalfEdge* twinEdge = edge->twin;
        Face* oppositeFace = twinEdge->face;

        if (oppositeFace->mark == Face::Visible) {
            if (oppositeFace->distanceToPoint(eyePoint) > this->tolerance) {
                // the opposite face can see the vertex, so proceed with next edge

                this->_computeHorizon(eyePoint, twinEdge, oppositeFace, horizon);
            } else {
                // the opposite face can't see the vertex, so this edge is part of the horizon

                horizon.push_back(edge);
            }
        }

        edge = edge->next;

    } while (edge != crossEdge);

    return *this;
}

HalfEdge* ConvexHull::_addAdjoiningFace(VertexNode* eyeVertex, HalfEdge* horizonEdge) {
    // all the half edges are created in ccw order thus the face is always pointing outside the
    // hull

    Face* face = Face::create(this->arena_, eyeVertex, horizonEdge->tail(), horizonEdge->head());

    this->faces.push_back(face);

    // join face.getEdge( - 1 ) with the horizon's opposite edge face.getEdge( - 1 ) =
    // face.getEdge( 2 )

    face->getEdge(-1)->setTwin(horizonEdge->twin);

    return face->getEdge(0); // the half edge whose vertex is the eyeVertex
}

ConvexHull& ConvexHull::_addNewFaces(VertexNode* eyeVertex, const std::vector<HalfEdge*>& horizon) {
    this->newFaces.clear();

    HalfEdge* firstSideEdge = nullptr;
    HalfEdge* previousSideEdge = nullptr;

    for (std::size_t i = 0; i < horizon.size(); i++) {
        HalfEdge* horizonEdge = horizon[i];

        // returns the right side edge

        HalfEdge* sideEdge = this->_addAdjoiningFace(eyeVertex, horizonEdge);

        if (firstSideEdge == nullptr) {
            firstSideEdge = sideEdge;
        } else {
            // joins face.getEdge( 1 ) with previousFace.getEdge( 0 )

            sideEdge->next->setTwin(previousSideEdge);
        }

        this->newFaces.push_back(sideEdge->face);
        previousSideEdge = sideEdge;
    }

    // perform final join of new faces

    firstSideEdge->next->setTwin(previousSideEdge);

    return *this;
}

ConvexHull& ConvexHull::_addVertexToHull(VertexNode* eyeVertex) {
    std::vector<HalfEdge*> horizon;

    this->unassigned.clear();

    // remove 'eyeVertex' from 'eyeVertex.face' so that it can't be added to the 'unassigned'
    // vertex list

    this->_removeVertexFromFace(eyeVertex, eyeVertex->face);

    this->_computeHorizon(eyeVertex->point, nullptr, eyeVertex->face, horizon);

    this->_addNewFaces(eyeVertex, horizon);

    // reassign 'unassigned' vertices to the new faces

    this->_resolveUnassignedPoints(this->newFaces);

    // the source writes `return this;`, which returns the hull itself
    return *this;
}

ConvexHull& ConvexHull::_cleanup() {
    this->assigned.clear();
    this->unassigned.clear();
    this->newFaces.clear();

    return *this;
}

ConvexHull& ConvexHull::_compute() {
    VertexNode* vertex;

    this->_computeInitialHull();

    // add all available vertices gradually to the hull

    while ((vertex = this->_nextVertexToAdd()) != nullptr) {
        this->_addVertexToHull(vertex);
    }

    this->_reindexFaces();

    this->_cleanup();

    return *this;
}
