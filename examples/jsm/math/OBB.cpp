#include "math/OBB.h"
#include "math/Box3.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Ray.h"
#include "math/Sphere.h"
#include <cmath>

namespace {

// Matrix3::extractBasis() of the existing port is not const-qualified, so the same three
// column reads are repeated here to keep the query methods of OBB const-correct.
void extractBasis(const Matrix3& rotation, Vector3& xAxis, Vector3& yAxis, Vector3& zAxis) {
    xAxis.setFromMatrix3Column(rotation, 0);
    yAxis.setFromMatrix3Column(rotation, 1);
    zAxis.setFromMatrix3Column(rotation, 2);
}

// const a = { c: null, u: [ new Vector3(), new Vector3(), new Vector3() ], e: [] };
struct Axes {
    const Vector3* c = nullptr; // center
    Vector3 u[3];               // basis vectors
    double e[3] = {0, 0, 0};    // half width
};

// The module scope helper variables of the source, one set per owning thread. `a`, `b`, `R`,
// `AbsR` and `t` belong to intersectsOBB(); the remaining objects are shared by the other
// methods exactly like the upstream module scope.
struct ModuleTemporaries {
    Axes a;
    Axes b;
    double R[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    double AbsR[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    double t[3] = {0, 0, 0};
    Vector3 xAxis;
    Vector3 yAxis;
    Vector3 zAxis;
    Vector3 v1;
    Vector3 size;
    Vector3 closestPoint;
    Matrix3 rotationMatrix;
    Box3 aabb;
    Matrix4 matrix;
    Matrix4 inverse;
    Ray localRay;
    OBB obb;
};

ModuleTemporaries& temporaries() {
    static thread_local ModuleTemporaries value;
    return value;
}

} // namespace

// Parameter names of the definitions are shortened the way the existing math classes do
// (see Sphere.cpp) so that no parameter shadows the member of the same upstream name.
OBB::OBB(const Vector3& c, const Vector3& h, const Matrix3& r) : center(c), halfSize(h), rotation(r) {}

OBB& OBB::set(const Vector3& c, const Vector3& h, const Matrix3& r) {
    this->center.copy(c);
    this->halfSize.copy(h);
    this->rotation.copy(r);

    return *this;
}

OBB& OBB::copy(const OBB& obb) {
    this->center.copy(obb.center);
    this->halfSize.copy(obb.halfSize);
    this->rotation.copy(obb.rotation);

    return *this;
}

OBB OBB::clone() const {
    return OBB().copy(*this);
}

Vector3& OBB::getSize(Vector3& target) const {
    return target.copy(this->halfSize).multiplyScalar(2);
}

Vector3& OBB::clampPoint(const Vector3& point, Vector3& target) const {
    // Reference: Closest Point on OBB to Point in Real-Time Collision Detection
    // by Christer Ericson (chapter 5.1.4)

    ModuleTemporaries& m = temporaries();
    const Vector3& extents = this->halfSize;

    m.v1.subVectors(point, this->center);
    extractBasis(this->rotation, m.xAxis, m.yAxis, m.zAxis);

    // start at the center position of the OBB

    target.copy(this->center);

    // project the target onto the OBB axes and walk towards that point

    const double x = MathUtils::clamp(m.v1.dot(m.xAxis), -extents.x, extents.x);
    target.add(m.xAxis.multiplyScalar(x));

    const double y = MathUtils::clamp(m.v1.dot(m.yAxis), -extents.y, extents.y);
    target.add(m.yAxis.multiplyScalar(y));

    const double z = MathUtils::clamp(m.v1.dot(m.zAxis), -extents.z, extents.z);
    target.add(m.zAxis.multiplyScalar(z));

    return target;
}

bool OBB::containsPoint(const Vector3& point) const {
    ModuleTemporaries& m = temporaries();

    m.v1.subVectors(point, this->center);
    extractBasis(this->rotation, m.xAxis, m.yAxis, m.zAxis);

    // project v1 onto each axis and check if these points lie inside the OBB

    return std::abs(m.v1.dot(m.xAxis)) <= this->halfSize.x &&
           std::abs(m.v1.dot(m.yAxis)) <= this->halfSize.y &&
           std::abs(m.v1.dot(m.zAxis)) <= this->halfSize.z;
}

bool OBB::intersectsBox3(const Box3& box3) const {
    ModuleTemporaries& m = temporaries();

    return this->intersectsOBB(m.obb.fromBox3(box3));
}

bool OBB::intersectsSphere(const Sphere& sphere) const {
    ModuleTemporaries& m = temporaries();

    // find the point on the OBB closest to the sphere center

    this->clampPoint(sphere.center, m.closestPoint);

    // if that point is inside the sphere, the OBB and sphere intersect

    return m.closestPoint.distanceToSquared(sphere.center) <= (sphere.radius * sphere.radius);
}

bool OBB::intersectsOBB(const OBB& obb, double epsilon) const {
    // Reference: OBB-OBB Intersection in Real-Time Collision Detection
    // by Christer Ericson (chapter 4.4.1)

    // prepare data structures (the code uses the same nomenclature like the reference)

    ModuleTemporaries& m = temporaries();

    m.a.c = &this->center;
    m.a.e[0] = this->halfSize.x;
    m.a.e[1] = this->halfSize.y;
    m.a.e[2] = this->halfSize.z;
    extractBasis(this->rotation, m.a.u[0], m.a.u[1], m.a.u[2]);

    m.b.c = &obb.center;
    m.b.e[0] = obb.halfSize.x;
    m.b.e[1] = obb.halfSize.y;
    m.b.e[2] = obb.halfSize.z;
    extractBasis(obb.rotation, m.b.u[0], m.b.u[1], m.b.u[2]);

    // compute rotation matrix expressing b in a's coordinate frame

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m.R[i][j] = m.a.u[i].dot(m.b.u[j]);
        }
    }

    // compute translation vector

    m.v1.subVectors(*m.b.c, *m.a.c);

    // bring translation into a's coordinate frame

    m.t[0] = m.v1.dot(m.a.u[0]);
    m.t[1] = m.v1.dot(m.a.u[1]);
    m.t[2] = m.v1.dot(m.a.u[2]);

    // compute common subexpressions. Add in an epsilon term to
    // counteract arithmetic errors when two edges are parallel and
    // their cross product is (near) null

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m.AbsR[i][j] = std::abs(m.R[i][j]) + epsilon;
        }
    }

    double ra, rb;

    // test axes L = A0, L = A1, L = A2

    for (int i = 0; i < 3; ++i) {
        ra = m.a.e[i];
        rb = m.b.e[0] * m.AbsR[i][0] + m.b.e[1] * m.AbsR[i][1] + m.b.e[2] * m.AbsR[i][2];
        if (std::abs(m.t[i]) > ra + rb) return false;
    }

    // test axes L = B0, L = B1, L = B2

    for (int i = 0; i < 3; ++i) {
        ra = m.a.e[0] * m.AbsR[0][i] + m.a.e[1] * m.AbsR[1][i] + m.a.e[2] * m.AbsR[2][i];
        rb = m.b.e[i];
        if (std::abs(m.t[0] * m.R[0][i] + m.t[1] * m.R[1][i] + m.t[2] * m.R[2][i]) > ra + rb) return false;
    }

    // test axis L = A0 x B0

    ra = m.a.e[1] * m.AbsR[2][0] + m.a.e[2] * m.AbsR[1][0];
    rb = m.b.e[1] * m.AbsR[0][2] + m.b.e[2] * m.AbsR[0][1];
    if (std::abs(m.t[2] * m.R[1][0] - m.t[1] * m.R[2][0]) > ra + rb) return false;

    // test axis L = A0 x B1

    ra = m.a.e[1] * m.AbsR[2][1] + m.a.e[2] * m.AbsR[1][1];
    rb = m.b.e[0] * m.AbsR[0][2] + m.b.e[2] * m.AbsR[0][0];
    if (std::abs(m.t[2] * m.R[1][1] - m.t[1] * m.R[2][1]) > ra + rb) return false;

    // test axis L = A0 x B2

    ra = m.a.e[1] * m.AbsR[2][2] + m.a.e[2] * m.AbsR[1][2];
    rb = m.b.e[0] * m.AbsR[0][1] + m.b.e[1] * m.AbsR[0][0];
    if (std::abs(m.t[2] * m.R[1][2] - m.t[1] * m.R[2][2]) > ra + rb) return false;

    // test axis L = A1 x B0

    ra = m.a.e[0] * m.AbsR[2][0] + m.a.e[2] * m.AbsR[0][0];
    rb = m.b.e[1] * m.AbsR[1][2] + m.b.e[2] * m.AbsR[1][1];
    if (std::abs(m.t[0] * m.R[2][0] - m.t[2] * m.R[0][0]) > ra + rb) return false;

    // test axis L = A1 x B1

    ra = m.a.e[0] * m.AbsR[2][1] + m.a.e[2] * m.AbsR[0][1];
    rb = m.b.e[0] * m.AbsR[1][2] + m.b.e[2] * m.AbsR[1][0];
    if (std::abs(m.t[0] * m.R[2][1] - m.t[2] * m.R[0][1]) > ra + rb) return false;

    // test axis L = A1 x B2

    ra = m.a.e[0] * m.AbsR[2][2] + m.a.e[2] * m.AbsR[0][2];
    rb = m.b.e[0] * m.AbsR[1][1] + m.b.e[1] * m.AbsR[1][0];
    if (std::abs(m.t[0] * m.R[2][2] - m.t[2] * m.R[0][2]) > ra + rb) return false;

    // test axis L = A2 x B0

    ra = m.a.e[0] * m.AbsR[1][0] + m.a.e[1] * m.AbsR[0][0];
    rb = m.b.e[1] * m.AbsR[2][2] + m.b.e[2] * m.AbsR[2][1];
    if (std::abs(m.t[1] * m.R[0][0] - m.t[0] * m.R[1][0]) > ra + rb) return false;

    // test axis L = A2 x B1

    ra = m.a.e[0] * m.AbsR[1][1] + m.a.e[1] * m.AbsR[0][1];
    rb = m.b.e[0] * m.AbsR[2][2] + m.b.e[2] * m.AbsR[2][0];
    if (std::abs(m.t[1] * m.R[0][1] - m.t[0] * m.R[1][1]) > ra + rb) return false;

    // test axis L = A2 x B2

    ra = m.a.e[0] * m.AbsR[1][2] + m.a.e[1] * m.AbsR[0][2];
    rb = m.b.e[0] * m.AbsR[2][1] + m.b.e[1] * m.AbsR[2][0];
    if (std::abs(m.t[1] * m.R[0][2] - m.t[0] * m.R[1][2]) > ra + rb) return false;

    // since no separating axis is found, the OBBs must be intersecting

    return true;
}

bool OBB::intersectsPlane(const Plane& plane) const {
    // Reference: Testing Box Against Plane in Real-Time Collision Detection
    // by Christer Ericson (chapter 5.2.3)

    ModuleTemporaries& m = temporaries();

    extractBasis(this->rotation, m.xAxis, m.yAxis, m.zAxis);

    // compute the projection interval radius of this OBB onto L(t) = this->center + t * p.normal;

    const double r = this->halfSize.x * std::abs(plane.normal.dot(m.xAxis)) +
                     this->halfSize.y * std::abs(plane.normal.dot(m.yAxis)) +
                     this->halfSize.z * std::abs(plane.normal.dot(m.zAxis));

    // compute distance of the OBB's center from the plane

    const double d = plane.normal.dot(this->center) - plane.constant;

    // Intersection occurs when distance d falls within [-r,+r] interval

    return std::abs(d) <= r;
}

Vector3* OBB::intersectRay(const Ray& ray, Vector3& target) const {
    // the idea is to perform the intersection test in the local space of the OBB.

    ModuleTemporaries& m = temporaries();

    this->getSize(m.size);
    m.v1.set(0, 0, 0);
    m.aabb.setFromCenterAndSize(m.v1, m.size);

    // create a 4x4 transformation matrix

    m.matrix.setFromMatrix3(this->rotation);
    m.matrix.setPosition(this->center);

    // transform ray to the local space of the OBB

    m.inverse.copy(m.matrix).invert();
    m.localRay.copy(ray).applyMatrix4(m.inverse);

    // perform ray <-> AABB intersection test

    if (m.localRay.intersectBox(m.aabb, target) != nullptr) {
        // transform the intersection point back to world space

        return &target.applyMatrix4(m.matrix);
    }

    return nullptr;
}

bool OBB::intersectsRay(const Ray& ray) const {
    ModuleTemporaries& m = temporaries();

    return this->intersectRay(ray, m.v1) != nullptr;
}

OBB& OBB::fromBox3(const Box3& box3) {
    box3.getCenter(this->center);

    box3.getSize(this->halfSize).multiplyScalar(0.5);

    this->rotation.identity();

    return *this;
}

bool OBB::equals(const OBB& obb) const {
    return obb.center.equals(this->center) && obb.halfSize.equals(this->halfSize) &&
           obb.rotation.equals(this->rotation);
}

OBB& OBB::applyMatrix4(const Matrix4& matrix) {
    ModuleTemporaries& m = temporaries();
    const auto e = matrix.elements();

    double sx = m.v1.set(e[0], e[1], e[2]).length();
    const double sy = m.v1.set(e[4], e[5], e[6]).length();
    const double sz = m.v1.set(e[8], e[9], e[10]).length();

    const double det = matrix.determinant();
    if (det < 0) sx = -sx;

    m.rotationMatrix.setFromMatrix4(matrix);

    const double invSX = 1 / sx;
    const double invSY = 1 / sy;
    const double invSZ = 1 / sz;

    auto rotationElements = m.rotationMatrix.elements();
    rotationElements[0] *= invSX;
    rotationElements[1] *= invSX;
    rotationElements[2] *= invSX;

    rotationElements[3] *= invSY;
    rotationElements[4] *= invSY;
    rotationElements[5] *= invSY;

    rotationElements[6] *= invSZ;
    rotationElements[7] *= invSZ;
    rotationElements[8] *= invSZ;

    this->rotation.multiply(m.rotationMatrix);

    this->halfSize.x *= sx;
    this->halfSize.y *= sy;
    this->halfSize.z *= sz;

    m.v1.setFromMatrixPosition(matrix);
    this->center.add(m.v1);

    return *this;
}
