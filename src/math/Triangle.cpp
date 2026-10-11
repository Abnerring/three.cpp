#include "math/Triangle.h"
#include "core/BufferAttribute.h"
#include "math/Box3.h"
#include "math/Plane.h"
#include <cmath>

Vector3& Triangle::getNormal(const Vector3& a, const Vector3& b, const Vector3& c,
                             Vector3& target) {
    Vector3 _v0;
    target.subVectors(c, b);
    _v0.subVectors(a, b);
    target.cross(_v0);
    const auto targetLengthSq = target.lengthSq();
    if (targetLengthSq > 0) {
        return target.multiplyScalar(1 / std::sqrt(targetLengthSq));
    }
    return target.set(0, 0, 0);
}

Vector3* Triangle::getBarycoord(const Vector3& point, const Vector3& a, const Vector3& b,
                                const Vector3& c, Vector3& target) {
    Vector3 _v0, _v1, _v2;
    _v0.subVectors(c, a);
    _v1.subVectors(b, a);
    _v2.subVectors(point, a);
    const auto dot00 = _v0.dot(_v0);
    const auto dot01 = _v0.dot(_v1);
    const auto dot02 = _v0.dot(_v2);
    const auto dot11 = _v1.dot(_v1);
    const auto dot12 = _v1.dot(_v2);
    const auto denom = (dot00 * dot11 - dot01 * dot01);
    if (denom == 0) {
        target.set(0, 0, 0);
        return nullptr;
    }
    const auto invDenom = 1 / denom;
    const auto u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    const auto v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    return &target.set(1 - u - v, v, u);
}

bool Triangle::containsPoint(const Vector3& point, const Vector3& a, const Vector3& b,
                             const Vector3& c) {
    Vector3 _v3;
    if (Triangle::getBarycoord(point, a, b, c, _v3) == nullptr) {
        return false;
    }
    return (_v3.x() >= 0) && (_v3.y() >= 0) && ((_v3.x() + _v3.y()) <= 1);
}

bool Triangle::isFrontFacing(const Vector3& a, const Vector3& b, const Vector3& c,
                             const Vector3& direction) {
    Vector3 _v0, _v1;
    _v0.subVectors(c, b);
    _v1.subVectors(a, b);
    return _v0.cross(_v1).dot(direction) < 0;
}

Triangle& Triangle::set(const Vector3& a, const Vector3& b, const Vector3& c) {
    this->a.copy(a);
    this->b.copy(b);
    this->c.copy(c);
    return *this;
}

Triangle& Triangle::setFromAttributeAndIndices(const BufferAttribute& attribute, size_t i0,
                                               size_t i1, size_t i2) {
    this->a.fromBufferAttribute(attribute, i0);
    this->b.fromBufferAttribute(attribute, i1);
    this->c.fromBufferAttribute(attribute, i2);
    return *this;
}

Triangle& Triangle::copy(const Triangle& triangle) {
    this->a.copy(triangle.a);
    this->b.copy(triangle.b);
    this->c.copy(triangle.c);
    return *this;
}

HIGH_PRECISION Triangle::getArea() const {
    Vector3 _v0, _v1;
    _v0.subVectors(this->c, this->b);
    _v1.subVectors(this->a, this->b);
    return _v0.cross(_v1).length() * 0.5;
}

Vector3& Triangle::getMidpoint(Vector3& target) const {
    return target.addVectors(this->a, this->b).add(this->c).multiplyScalar(1.0 / 3);
}

Vector3& Triangle::getNormal(Vector3& target) const {
    return Triangle::getNormal(this->a, this->b, this->c, target);
}

Plane& Triangle::getPlane(Plane& target) const {
    return target.setFromCoplanarPoints(this->a, this->b, this->c);
}

Vector3* Triangle::getBarycoord(const Vector3& point, Vector3& target) const {
    return Triangle::getBarycoord(point, this->a, this->b, this->c, target);
}

bool Triangle::containsPoint(const Vector3& point) const {
    return Triangle::containsPoint(point, this->a, this->b, this->c);
}

bool Triangle::isFrontFacing(const Vector3& direction) const {
    return Triangle::isFrontFacing(this->a, this->b, this->c, direction);
}

bool Triangle::intersectsBox(const Box3& box) const {
    return box.intersectsTriangle(*this);
}

Vector3& Triangle::closestPointToPoint(const Vector3& p, Vector3& target) const {
    Vector3 _vab, _vac, _vbc, _vap, _vbp, _vcp;
    const auto& a = this->a;
    const auto& b = this->b;
    const auto& c = this->c;
    HIGH_PRECISION v, w;
    _vab.subVectors(b, a);
    _vac.subVectors(c, a);
    _vap.subVectors(p, a);
    const auto d1 = _vab.dot(_vap);
    const auto d2 = _vac.dot(_vap);
    if (d1 <= 0 && d2 <= 0) {
        return target.copy(a);
    }
    _vbp.subVectors(p, b);
    const auto d3 = _vab.dot(_vbp);
    const auto d4 = _vac.dot(_vbp);
    if (d3 >= 0 && d4 <= d3) {
        return target.copy(b);
    }
    const auto vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0 && d1 - d3 > 0) {
        v = d1 / (d1 - d3);
        return target.copy(a).addScaledVector(_vab, v);
    }
    _vcp.subVectors(p, c);
    const auto d5 = _vab.dot(_vcp);
    const auto d6 = _vac.dot(_vcp);
    if (d6 >= 0 && d5 <= d6) {
        return target.copy(c);
    }
    const auto vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) {
        w = d2 / (d2 - d6);
        return target.copy(a).addScaledVector(_vac, w);
    }
    const auto va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
        _vbc.subVectors(c, b);
        w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return target.copy(b).addScaledVector(_vbc, w);
    }
    const auto denom = 1 / (va + vb + vc);
    v = vb * denom;
    w = vc * denom;
    return target.copy(a).addScaledVector(_vab, v).addScaledVector(_vac, w);
}

bool Triangle::equals(const Triangle& triangle) const {
    return triangle.a.equals(this->a) && triangle.b.equals(this->b) && triangle.c.equals(this->c);
}

Triangle::Triangle(const Vector3& a, const Vector3& b, const Vector3& c) : a(a), b(b), c(c) {}
Triangle Triangle::clone() const {
    return Triangle().copy(*this);
}
Triangle& Triangle::setFromPointsAndIndices(const std::vector<Vector3>& points, size_t i0,
                                            size_t i1, size_t i2) {
    a.copy(points.at(i0));
    b.copy(points.at(i1));
    c.copy(points.at(i2));
    return *this;
}
namespace {
template <class V> V attributeVector(const Vector4& source) {
    V value;
    value.setX(source.x()).setY(source.y());
    if constexpr (requires { value.setZ(0); })
        value.setZ(source.z());
    if constexpr (requires { value.setW(0); })
        value.setW(source.w());
    return value;
}
template <class V>
V& interpolateAttribute(const BufferAttribute& attr, size_t i1, size_t i2, size_t i3,
                        const Vector3& barycoord, V& target) {
    Vector4 v40, v41, v42;
    v40.setScalar(0);
    v41.setScalar(0);
    v42.setScalar(0);
    v40.fromBufferAttribute(attr, i1);
    v41.fromBufferAttribute(attr, i2);
    v42.fromBufferAttribute(attr, i3);
    target.setScalar(0);
    target.addScaledVector(attributeVector<V>(v40), barycoord.x());
    target.addScaledVector(attributeVector<V>(v41), barycoord.y());
    target.addScaledVector(attributeVector<V>(v42), barycoord.z());
    return target;
}
} // namespace
Vector2& Triangle::getInterpolatedAttribute(const BufferAttribute& attr, size_t i1, size_t i2,
                                            size_t i3, const Vector3& barycoord, Vector2& target) {
    return interpolateAttribute(attr, i1, i2, i3, barycoord, target);
}
Vector3& Triangle::getInterpolatedAttribute(const BufferAttribute& attr, size_t i1, size_t i2,
                                            size_t i3, const Vector3& barycoord, Vector3& target) {
    return interpolateAttribute(attr, i1, i2, i3, barycoord, target);
}
Vector4& Triangle::getInterpolatedAttribute(const BufferAttribute& attr, size_t i1, size_t i2,
                                            size_t i3, const Vector3& barycoord, Vector4& target) {
    return interpolateAttribute(attr, i1, i2, i3, barycoord, target);
}
