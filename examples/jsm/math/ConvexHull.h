#pragma once
#include "math/Vector3.h"
#include <cstddef>
#include <memory>
#include <vector>

class HalfEdge;
class VertexNode;
class Face;
struct ConvexHullArena;
class Ray;
class Object3D;

// Native port of examples/jsm/math/ConvexHull.js from the pinned three.js revision (see
// upstream.json): the quickhull3d implementation of Mauricio Poppe plus the four helper
// classes Face, HalfEdge, VertexNode and VertexList, which the pinned module exports as well.
//
// Ownership: the source relies on the garbage collector, so the doubly linked lists and the
// half-edge back references (`face.edge.next.face`, `vertex.face.outside`, `edge.twin`) form
// cycles that are never released explicitly. The native port gives every ConvexHull exactly
// one ConvexHullArena that owns its Face, HalfEdge and VertexNode objects and keeps the
// upstream links as raw pointers. That preserves the identity, the traversal order and the
// cycle structure of the source instead of rewriting them with shared_ptr/weak_ptr pairs, and
// it keeps `Face`, `HalfEdge` and `VertexNode` usable as plain declarations. Nodes live until
// the hull itself is destroyed; `makeEmpty()` only drops the hull arrays, so an object a
// caller still referenced stays valid like it does in the source.
//
// The module scope temporaries `_v1`, `_line3`, `_plane`, `_closestPoint` and `_triangle` of
// the source become one thread_local set in ConvexHull.cpp (project convention).

/** A vertex as a double linked list node. */
class VertexNode {
  public:
    /** A point in 3D space (the source stores the given Vector3; the port copies it). */
    Vector3 point;

    /** Reference to the previous vertex in the double linked list. */
    VertexNode* prev = nullptr;

    /** Reference to the next vertex in the double linked list. */
    VertexNode* next = nullptr;

    /** Reference to the face that is able to see this vertex. */
    Face* face = nullptr;

    explicit VertexNode(const Vector3& point);
};

/** The basis for a half-edge data structure, also known as doubly connected edge list. */
class HalfEdge {
  public:
    /** A reference to its destination vertex. */
    VertexNode* vertex = nullptr;

    /** Reference to the previous half-edge of the same face. */
    HalfEdge* prev = nullptr;

    /** Reference to the next half-edge of the same face. */
    HalfEdge* next = nullptr;

    /** Reference to the twin half-edge to reach the opposite face. */
    HalfEdge* twin = nullptr;

    /** A reference to its face. */
    Face* face = nullptr;

    HalfEdge(VertexNode* vertex, Face* face);

    /** Returns the destination vertex. */
    VertexNode* head() const;

    /** Returns the origin vertex, or nullptr when the edge has no previous edge. */
    VertexNode* tail() const;

    /** Returns the Euclidean length of the edge, or -1 without a tail vertex. */
    double length() const;

    /** Returns the squared Euclidean length of the edge, or -1 without a tail vertex. */
    double lengthSquared() const;

    /** Sets the twin edge; the given edge points back to this one. */
    HalfEdge* setTwin(HalfEdge* edge);
};

/** A doubly linked list of vertices. */
class VertexList {
  public:
    /** Reference to the first vertex of the linked list. */
    VertexNode* head = nullptr;

    /** Reference to the last vertex of the linked list. */
    VertexNode* tail = nullptr;

    /** Returns the head reference. */
    VertexNode* first() const;

    /** Returns the tail reference. */
    VertexNode* last() const;

    /** Clears the linked list. */
    VertexList& clear();

    /** Inserts a vertex before a target vertex. */
    VertexList& insertBefore(VertexNode* target, VertexNode* vertex);

    /** Inserts a vertex after a target vertex. */
    VertexList& insertAfter(VertexNode* target, VertexNode* vertex);

    /** Appends a vertex to this vertex list. */
    VertexList& append(VertexNode* vertex);

    /** Appends a chain of vertices where the given vertex is the head. */
    VertexList& appendChain(VertexNode* vertex);

    /** Removes a vertex from the linked list. */
    VertexList& remove(VertexNode* vertex);

    /** Removes a sublist of vertices from the linked list. */
    VertexList& removeSubList(VertexNode* a, VertexNode* b);

    /** Returns `true` if the linked list is empty. */
    bool isEmpty() const;
};

/**
 * Represents a section bounded by a specific amount of half-edges.
 * The current implementation assumes that a face always consist of three edges.
 */
class Face {
  public:
    /** The face is part of the hull and can still be seen by a point. */
    static constexpr int Visible = 0;

    /** The face was removed from the hull by the current iteration. */
    static constexpr int Deleted = 1;

    /** The normal vector of the face. */
    Vector3 normal;

    /** The midpoint or centroid of the face. */
    Vector3 midpoint;

    /** The area of the face. */
    double area = 0;

    /** Signed distance from face to the origin. */
    double constant = 0;

    /** Reference to a vertex in a vertex list this face can see. */
    VertexNode* outside = nullptr;

    /** Whether the face is visible (part of the hull) or deleted. */
    int mark = Visible;

    /** Reference to the base edge of a face. */
    HalfEdge* edge = nullptr;

    Face() = default;

    /**
     * Creates a face from the given vertex nodes. The source allocates through the JavaScript
     * garbage collector; the port allocates through the arena that owns the hull.
     */
    static Face* create(ConvexHullArena& arena, VertexNode* a, VertexNode* b, VertexNode* c);

    /** Returns an edge by the given index, counted around the face. */
    HalfEdge* getEdge(int i) const;

    /** Computes all properties of the face. */
    Face& compute();

    /** Returns the signed distance from a given point to the plane representation of this face. */
    double distanceToPoint(const Vector3& point) const;
};

/**
 * Owns every Face, HalfEdge and VertexNode of one hull. The source has no counterpart: the
 * JavaScript garbage collector reclaims those objects as soon as the hull no longer reaches
 * them.
 */
struct ConvexHullArena {
    std::vector<std::unique_ptr<Face>> faces;
    std::vector<std::unique_ptr<HalfEdge>> halfEdges;
    std::vector<std::unique_ptr<VertexNode>> vertexNodes;

    Face* createFace();
    HalfEdge* createHalfEdge(VertexNode* vertex, Face* face);
    VertexNode* createVertexNode(const Vector3& point);
};

/**
 * Can be used to compute the convex hull in 3D space for a given set of points. It is primarily
 * intended for ConvexGeometry.
 */
class ConvexHull {
  public:
    /** Three times Number.EPSILON times the size of the extreme points; -1 before a compute. */
    double tolerance = -1;

    /** The generated faces of the convex hull. */
    std::vector<Face*> faces;

    /** The faces that are generated within a single iteration. */
    std::vector<Face*> newFaces;

    /** Vertices that are assigned to a face, see the layout comment in the source. */
    VertexList assigned;

    /** Vertices that are currently not assigned to a face. */
    VertexList unassigned;

    /** Vertices of the hull (internal representation of given geometry data). */
    std::vector<VertexNode*> vertices;

    ConvexHull();
    // The nodes of one hull are linked by raw pointers into its arena, so a hull cannot be
    // copied (docs/PORTING.md keeps copy/clone for objects that support it upstream; the
    // source has neither for ConvexHull either).
    ConvexHull(const ConvexHull&) = delete;
    ConvexHull& operator=(const ConvexHull&) = delete;

    /** Computes the convex hull for the given array of points. */
    ConvexHull& setFromPoints(const std::vector<Vector3>& points);

    /**
     * Computes the convex hull of the given 3D object (including its descendants), accounting
     * for the world transforms of both the 3D object and its descendants.
     */
    ConvexHull& setFromObject(Object3D& object);

    /** Returns `true` if the given point lies in the convex hull. */
    bool containsPoint(const Vector3& point) const;

    /**
     * Computes the intersection point of the given ray and this convex hull. Returns the
     * caller's target on a hit and nullptr on a miss, the nullable convention of Ray.
     */
    Vector3* intersectRay(const Ray& ray, Vector3& target) const;

    /** Returns `true` if the given ray intersects with this convex hull. */
    bool intersectsRay(const Ray& ray) const;

    /** Makes the convex hull empty. */
    ConvexHull& makeEmpty();

    // The following methods are documented `@private` in the pinned source but stay reachable
    // there; the native port keeps their names and their public reachability.

    /** Adds a vertex to the 'assigned' list of vertices and assigns it to the given face. */
    ConvexHull& _addVertexToFace(VertexNode* vertex, Face* face);

    /**
     * Removes a vertex from the 'assigned' list of vertices and from the given face, and fixes
     * the link from 'face' to the first vertex it sees in 'assigned'.
     */
    ConvexHull& _removeVertexFromFace(VertexNode* vertex, Face* face);

    /**
     * Removes all the visible vertices that a given face is able to see which are stored in the
     * 'assigned' vertex list. Returns nullptr when the face sees no vertex.
     */
    VertexNode* _removeAllVerticesFromFace(Face* face);

    /**
     * Removes all the visible vertices that `face` is able to see. Without an absorbing face the
     * removed vertices go to the 'unassigned' list, otherwise each vertex is assigned to the
     * absorbing face when it can see it.
     */
    ConvexHull& _deleteFaceVertices(Face* face, Face* absorbingFace = nullptr);

    /**
     * Reassigns as many vertices as possible from the unassigned list to the new faces. The
     * parameter is the upstream `newFaces`, renamed because it would hide the member.
     */
    ConvexHull& _resolveUnassignedPoints(const std::vector<Face*>& newFaceList);

    /** The extremes of the six axis directions, used to compute the initial hull. */
    struct Extremes {
        VertexNode* min[3] = {nullptr, nullptr, nullptr};
        VertexNode* max[3] = {nullptr, nullptr, nullptr};
    };

    /** Computes the extremes values (min/max vertices) used to compute the initial hull. */
    Extremes _computeExtremes();

    /** Computes the initial simplex assigning to its faces all candidate points. */
    ConvexHull& _computeInitialHull();

    /** Removes inactive (e.g. deleted) faces from the internal face list. */
    ConvexHull& _reindexFaces();

    /** Finds the next vertex to create faces with the current hull, nullptr when none is left. */
    VertexNode* _nextVertexToAdd();

    /**
     * Computes a chain of half edges in CCW order called the 'horizon'. For an edge to be part
     * of the horizon it must join a face that can see 'eyePoint' and a face that cannot.
     */
    ConvexHull& _computeHorizon(const Vector3& eyePoint, HalfEdge* crossEdge, Face* face,
                                std::vector<HalfEdge*>& horizon);

    /**
     * Creates a face with the vertices 'eyeVertex.point', 'horizonEdge.tail' and
     * 'horizonEdge.head' in CCW order and returns the half edge whose vertex is the eyeVertex.
     */
    HalfEdge* _addAdjoiningFace(VertexNode* eyeVertex, HalfEdge* horizonEdge);

    /**
     * Adds 'horizon.size()' faces to the hull, each face linked with the horizon opposite face
     * and the face on the left/right.
     */
    ConvexHull& _addNewFaces(VertexNode* eyeVertex, const std::vector<HalfEdge*>& horizon);

    /** Adds a vertex to the hull (horizon computation plus new faces). */
    ConvexHull& _addVertexToHull(VertexNode* eyeVertex);

    /** Cleans up internal properties after computing the convex hull. */
    ConvexHull& _cleanup();

    /** Starts the execution of the quick hull algorithm. */
    ConvexHull& _compute();

  private:
    ConvexHullArena arena_;
};
