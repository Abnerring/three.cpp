#include "Three.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
JsonValue number(double value) {
    if (std::isnan(value))
        return "NaN";
    if (std::isinf(value))
        return value > 0 ? "Infinity" : "-Infinity";
    if (value == 0 && std::signbit(value))
        return "-0";
    return value;
}
JsonValue vec(const Vector2& value) {
    return JsonValue::Array{number(value.x), number(value.y)};
}
JsonValue vec(const Vector3& value) {
    return JsonValue::Array{number(value.x), number(value.y), number(value.z)};
}
JsonValue spherical(const Spherical& value) {
    return JsonValue::Array{number(value.radius), number(value.phi), number(value.theta)};
}
JsonValue cylindrical(const Cylindrical& value) {
    return JsonValue::Array{number(value.radius), number(value.theta), number(value.y)};
}
JsonValue box(const Box2& value) {
    Vector2 center, size, parameter, clamped;
    const Vector2 point(4, -2);
    value.getCenter(center);
    value.getSize(size);
    value.getParameter(point, parameter);
    value.clampPoint(point, clamped);
    return JsonValue::Object{{"min", vec(value.min)},
                             {"max", vec(value.max)},
                             {"empty", value.isEmpty()},
                             {"center", vec(center)},
                             {"size", vec(size)},
                             {"parameter", vec(parameter)},
                             {"clamped", vec(clamped)},
                             {"distance", number(value.distanceToPoint(point))},
                             {"contains", value.containsPoint(point)},
                             {"containsSelf", value.containsBox(value)},
                             {"intersectsSelf", value.intersectsBox(value)}};
}
JsonValue matrix(const Matrix2& value) {
    JsonValue::Array result;
    for (double element : value.elements())
        result.emplace_back(number(element));
    return result;
}
} // namespace

int main() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    JsonValue output = JsonValue::Object{};
    output["sphericalDefault"] = spherical(Spherical());
    output["cylindricalDefault"] = cylindrical(Cylindrical());
    JsonValue::Array coordinates;
    for (const auto& point : std::vector<Vector3>{{0, 0, 0},
                                                  {2, -3, 4},
                                                  {-0., 0., -0.},
                                                  {0, 1, 0},
                                                  {0, -1, 0},
                                                  {1e200, -1e200, 1e200},
                                                  {1e-200, -1e-200, 1e-200},
                                                  {nan, 2, 3},
                                                  {INFINITY, 1, 0}}) {
        Spherical s;
        Cylindrical c;
        Vector3 sv, cv;
        s.setFromVector3(point);
        c.setFromVector3(point);
        sv.setFromSpherical(s);
        cv.setFromCylindrical(c);
        coordinates.emplace_back(JsonValue::Object{{"spherical", spherical(s)},
                                                   {"cylindrical", cylindrical(c)},
                                                   {"sphericalVector", vec(sv)},
                                                   {"cylindricalVector", vec(cv)}});
    }
    output["coordinates"] = std::move(coordinates);
    JsonValue::Array safe;
    for (double phi : {-1., 0., 0.0000001, 0.3, MATH_PI, MATH_PI + 1, double(INFINITY),
                       double(-INFINITY), nan}) {
        Spherical original(3, phi, -0.);
        auto copy = original.clone();
        copy.makeSafe();
        safe.emplace_back(
            JsonValue::Object{{"original", spherical(original)}, {"safe", spherical(copy)}});
    }
    output["safe"] = std::move(safe);
    Spherical s;
    s.set(2, 3, 4);
    output["sphericalCopy"] = spherical(Spherical().copy(s));
    Cylindrical c;
    c.set(5, 6, 7);
    output["cylindricalCopy"] = cylindrical(Cylindrical().copy(c.clone()));
    Matrix2 m;
    JsonValue::Array matrices{matrix(m)};
    m.set(1, 2, 3, 4);
    matrices.emplace_back(matrix(m));
    m.fromArray({-1, 5, 6, 7, 8}, 1);
    matrices.emplace_back(matrix(m));
    m.identity();
    matrices.emplace_back(matrix(m));
    matrices.emplace_back(matrix(Matrix2(-0., INFINITY, nan, -INFINITY)));
    output["matrices"] = std::move(matrices);
    JsonValue::Array boxes;
    Box2 b;
    boxes.emplace_back(box(b));
    b.setFromPoints({{-3, 2}, {4, -1}, {1, 5}});
    boxes.emplace_back(box(b));
    b.setFromCenterAndSize({2, -3}, {4, 6});
    boxes.emplace_back(box(b));
    auto clone = b.clone();
    clone.expandByPoint({-7, 9}).expandByVector({2, 3}).expandByScalar(0.25);
    boxes.emplace_back(box(clone));
    boxes.emplace_back(box(b));
    clone.intersect(Box2({-1, -1}, {1, 1}));
    boxes.emplace_back(box(clone));
    clone.intersect(Box2({5, 5}, {6, 6}));
    boxes.emplace_back(box(clone));
    clone.unionWith(b).translate({-2, 3});
    boxes.emplace_back(box(clone));
    b.set({1, 2}, {1, 2});
    boxes.emplace_back(box(b));
    b.setFromPoints({{nan, 1}, {0, nan}});
    boxes.emplace_back(box(b));
    b.setFromPoints({});
    boxes.emplace_back(box(b));
    output["boxes"] = std::move(boxes);
    Box2 p({0, 0}, {2, 2});
    output["boxRelations"] = JsonValue::Array{p.equals(p.clone()),
                                              p.containsBox(Box2({1, 1}, {2, 2})),
                                              p.containsBox(Box2({-1, 0}, {1, 1})),
                                              p.intersectsBox(Box2({2, 2}, {3, 3})),
                                              p.intersectsBox(Box2({2.000001, 2}, {3, 3})),
                                              p.copy(Box2()).isEmpty()};
    MeshBasicMaterial clipped;
    auto clipPlane = std::make_shared<Plane>(Vector3(0, 1, 0), -2);
    clipped.clippingPlanes = std::vector<std::shared_ptr<Plane>>{clipPlane};
    auto clippedCopy = clipped.clone();
    clipPlane->constant = -3;
    output["clippingPlanes"] = JsonValue::Object{
        {"source", clipped.toJSON().at("clippingPlanes")},
        {"clone", clippedCopy->toJSON().at("clippingPlanes")},
        {"independent", clippedCopy->clippingPlanes->at(0).get() != clipPlane.get()}};
    std::cout << output.dump() << '\n';
}
