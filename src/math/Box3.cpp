#include "math/Box3.h"
#include "core/VertexAttribute.h"
#include "math/MathUtils.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Sphere.h"
#include "math/Triangle.h"
#include "objects/SkinnedMesh.h"
#include "objects/InstancedMesh.h"
#include "objects/Line.h"
#include "objects/Points.h"
#include "objects/Sprite.h"
#include <cmath>
#include <span>
Box3::Box3(const Vector3& a, const Vector3& b) : min(a), max(b) {}
Box3& Box3::setFromObject(Object3D& object, bool precise) {
    makeEmpty(); return expandByObject(object, precise);
}
Box3& Box3::expandByObject(Object3D& object, bool precise) {
    static thread_local Vector3 vector;
    static thread_local Box3 box;
    object.updateWorldMatrix(false, false);
    auto* mesh = dynamic_cast<Mesh*>(&object);
    const auto geometry = mesh ? mesh->geometry : dynamic_cast<Line*>(&object) ? dynamic_cast<Line&>(object).geometry
        : dynamic_cast<Points*>(&object) ? dynamic_cast<Points&>(object).geometry
        : dynamic_cast<Sprite*>(&object) ? dynamic_cast<Sprite&>(object).geometry : nullptr;
    if (mesh || dynamic_cast<Line*>(&object) || dynamic_cast<Points*>(&object) || dynamic_cast<Sprite*>(&object)) {
        if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'getAttribute')");
        const auto position = geometry->getAttribute("position");
        if (precise && position && !object.isInstancedMesh) {
            const auto count = position->getCount();
            for (std::size_t i = 0; double(i) < count; ++i) {
                if (object.isMesh && mesh) mesh->getVertexPosition(i, vector);
                else vector.fromBufferAttribute(*position,double(i));
                vector.applyMatrix4(object.matrixWorld); expandByPoint(vector);
            }
        } else {
            if (auto* skinned = dynamic_cast<SkinnedMesh*>(mesh)) {
                if (!skinned->boundingBox) skinned->computeBoundingBox();
                if (!skinned->boundingBox) throw JSTypeError("Cannot read properties of null (reading 'min')");
                box.copy(*skinned->boundingBox);
            } else if (auto* instanced = dynamic_cast<InstancedMesh*>(mesh)) {
                if (!instanced->boundingBox) instanced->computeBoundingBox();
                if (!instanced->boundingBox) throw JSTypeError("Cannot read properties of null (reading 'min')");
                box.copy(*instanced->boundingBox);
            } else {
                if (!geometry->boundingBox) geometry->computeBoundingBox();
                if (!geometry->boundingBox) throw JSTypeError("Cannot read properties of null (reading 'min')");
                box.copy(*geometry->boundingBox);
            }
            box.applyMatrix4(object.matrixWorld); unionWith(box);
        }
    }
    const auto children = object.children;
    const auto count = children.size();
    for (std::size_t i = 0; i < count; ++i) {
        if (i >= children.size() || children.storage()->holes.contains(i) || !children[i])
            throw JSTypeError("Cannot read properties of undefined (reading 'updateWorldMatrix')");
        auto child = children[i]; expandByObject(*child, precise);
    }
    return *this;
}

namespace {
bool satForAxes(std::span<const double> axes, const Vector3& v0, const Vector3& v1,
                const Vector3& v2, const Vector3& extents) {
    Vector3 testAxis;
    for (std::size_t i = 0; i < axes.size(); i += 3) {
        testAxis.set(axes[i], axes[i + 1], axes[i + 2]);
        const double r = extents.x * std::abs(testAxis.x) +
                         extents.y * std::abs(testAxis.y) +
                         extents.z * std::abs(testAxis.z);
        const double p0 = v0.dot(testAxis), p1 = v1.dot(testAxis), p2 = v2.dot(testAxis);
        if (MathUtils::max(-MathUtils::max(MathUtils::max(p0, p1), p2),
                           MathUtils::min(MathUtils::min(p0, p1), p2)) > r)
            return false;
    }
    return true;
}
} // namespace

bool Box3::intersectsPlane(const Plane& plane) const {
    double minimum, maximum;
    if (plane.normal.x > 0) {
        minimum = plane.normal.x * min.x;
        maximum = plane.normal.x * max.x;
    } else {
        minimum = plane.normal.x * max.x;
        maximum = plane.normal.x * min.x;
    }
    if (plane.normal.y > 0) {
        minimum += plane.normal.y * min.y;
        maximum += plane.normal.y * max.y;
    } else {
        minimum += plane.normal.y * max.y;
        maximum += plane.normal.y * min.y;
    }
    if (plane.normal.z > 0) {
        minimum += plane.normal.z * min.z;
        maximum += plane.normal.z * max.z;
    } else {
        minimum += plane.normal.z * max.z;
        maximum += plane.normal.z * min.z;
    }
    return minimum <= -plane.constant && maximum >= -plane.constant;
}

bool Box3::intersectsTriangle(const Triangle& triangle) const {
    if (isEmpty())
        return false;
    Vector3 center, extents, v0, v1, v2, f0, f1, f2, triangleNormal;
    getCenter(center);
    extents.subVectors(max, center);
    v0.subVectors(triangle.a, center);
    v1.subVectors(triangle.b, center);
    v2.subVectors(triangle.c, center);
    f0.subVectors(v1, v0);
    f1.subVectors(v2, v1);
    f2.subVectors(v0, v2);
    const double edges[] = {0, -f0.z, f0.y, 0, -f1.z, f1.y, 0, -f2.z, f2.y,  f0.z,
                            0, -f0.x, f1.z, 0, -f1.x, f2.z, 0, -f2.x, -f0.y, f0.x,
                            0, -f1.y, f1.x, 0, -f2.y, f2.x, 0};
    if (!satForAxes(edges, v0, v1, v2, extents))
        return false;
    const double faces[] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
    if (!satForAxes(faces, v0, v1, v2, extents))
        return false;
    triangleNormal.crossVectors(f0, f1);
    const double normal[] = {triangleNormal.x, triangleNormal.y, triangleNormal.z};
    return satForAxes(normal, v0, v1, v2, extents);
}
Box3& Box3::set(const Vector3& a, const Vector3& b) {
    min.copy(a);
    max.copy(b);
    return *this;
}
Box3& Box3::copy(const Box3& b) {
    return set(b.min, b.max);
}
Box3 Box3::clone() const {
    return Box3(min, max);
}
Box3& Box3::makeEmpty() {
    min.set(INFINITY, INFINITY, INFINITY);
    max.set(-INFINITY, -INFINITY, -INFINITY);
    return *this;
}
bool Box3::isEmpty() const {
    return max.x < min.x || max.y < min.y || max.z < min.z;
}
Box3& Box3::setFromArray(const std::vector<double>& a) {
    makeEmpty();
    for (std::size_t i = 0; i + 2 < a.size(); i += 3)
        expandByPoint(Vector3(a[i], a[i + 1], a[i + 2]));
    return *this;
}
Box3& Box3::setFromPoints(const std::vector<Vector3>& p) {
    makeEmpty();
    for (const auto& v : p)
        expandByPoint(v);
    return *this;
}
Box3& Box3::setFromBufferAttribute(const VertexAttribute& a) {
    makeEmpty();
    Vector3 vector;
    for (std::size_t i = 0; i < a.getCount(); ++i) {
        vector.fromBufferAttribute(a, double(i));
        expandByPoint(vector);
    }
    return *this;
}
Box3& Box3::setFromCenterAndSize(const Vector3& c, const Vector3& s) {
    Vector3 half(s);
    half.multiplyScalar(.5);
    min.copy(c);
    min.sub(half);
    max.copy(c);
    max.add(half);
    return *this;
}
Vector3& Box3::getCenter(Vector3& t) const {
    if (isEmpty())
        t.set(0, 0, 0);
    else {
        t.addVectors(min, max);
        t.multiplyScalar(.5);
    }
    return t;
}
Vector3& Box3::getSize(Vector3& t) const {
    if (isEmpty())
        t.set(0, 0, 0);
    else
        t.subVectors(max, min);
    return t;
}
Box3& Box3::expandByPoint(const Vector3& p) {
    min.min(p);
    max.max(p);
    return *this;
}
Box3& Box3::expandByVector(const Vector3& v) {
    min.sub(v);
    max.add(v);
    return *this;
}
Box3& Box3::expandByScalar(double s) {
    min.addScalar(-s);
    max.addScalar(s);
    return *this;
}
bool Box3::containsPoint(const Vector3& p) const {
    return !(p.x < min.x || p.x > max.x || p.y < min.y || p.y > max.y ||
             p.z < min.z || p.z > max.z);
}
bool Box3::containsBox(const Box3& b) const {
    return min.x <= b.min.x && b.max.x <= max.x && min.y <= b.min.y &&
           b.max.y <= max.y && min.z <= b.min.z && b.max.z <= max.z;
}
bool Box3::intersectsBox(const Box3& b) const {
    return !(b.max.x < min.x || b.min.x > max.x || b.max.y < min.y ||
             b.min.y > max.y || b.max.z < min.z || b.min.z > max.z);
}
bool Box3::intersectsSphere(const Sphere& s) const {
    Vector3 v;
    clampPoint(s.center, v);
    return v.distanceToSquared(s.center) <= s.radius * s.radius;
}
Vector3& Box3::clampPoint(const Vector3& p, Vector3& t) const {
    t.copy(p);
    t.clamp(min, max);
    return t;
}
double Box3::distanceToPoint(const Vector3& p) const {
    Vector3 v;
    clampPoint(p, v);
    return v.distanceTo(p);
}
Vector3& Box3::getParameter(const Vector3& p, Vector3& t) const {
    t.set((p.x - min.x) / (max.x - min.x), (p.y - min.y) / (max.y - min.y),
          (p.z - min.z) / (max.z - min.z));
    return t;
}
Sphere& Box3::getBoundingSphere(Sphere& s) const {
    if (isEmpty())
        s.makeEmpty();
    else {
        Vector3 v;
        getCenter(s.center);
        s.radius = getSize(v).length() * .5;
    }
    return s;
}
Box3& Box3::intersect(const Box3& b) {
    min.max(b.min);
    max.min(b.max);
    if (isEmpty())
        makeEmpty();
    return *this;
}
Box3& Box3::unionWith(const Box3& b) {
    min.min(b.min);
    max.max(b.max);
    return *this;
}
Box3& Box3::applyMatrix4(const Matrix4& m) {
    if (isEmpty())
        return *this;
    const Vector3 a(min), b(max);
    makeEmpty();
    for (int i = 0; i < 8; ++i) {
        Vector3 v(i & 1 ? b.x : a.x, i & 2 ? b.y : a.y, i & 4 ? b.z : a.z);
        v.applyMatrix4(m);
        expandByPoint(v);
    }
    return *this;
}
Box3& Box3::translate(const Vector3& v) {
    min.add(v);
    max.add(v);
    return *this;
}
bool Box3::equals(const Box3& b) const {
    return min.equals(b.min) && max.equals(b.max);
}

// Upstream serialises the two corners as plain arrays; the same shape is produced here so a saved box
// round trips through fromJSON.
JSValue Box3::toJSON() const {
    auto output = JSValue::object();
    auto minArray = JSValue::array(), maxArray = JSValue::array();
    for (const double value : {min.x, min.y, min.z}) minArray.get("push").call(minArray, {JSValue(value)});
    for (const double value : {max.x, max.y, max.z}) maxArray.get("push").call(maxArray, {JSValue(value)});
    output.set("min", minArray);
    output.set("max", maxArray);
    return output;
}
Box3& Box3::fromJSON(const JSValue& json) {
    const auto read = [&](const char* name, Vector3& target) {
        const auto array = json.get(name);
        target.x = array.get("0").toNumber();
        target.y = array.get("1").toNumber();
        target.z = array.get("2").toNumber();
    };
    read("min", min);
    read("max", max);
    return *this;
}