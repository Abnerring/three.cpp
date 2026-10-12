#include "core/JsonValue.h"
#include "core/BufferAttribute.h"
#include "geometries/BoxGeometry.h"
#include "math/Box3.h"
#include "math/Frustum.h"
#include "math/Line3.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/Ray.h"
#include "math/Sphere.h"
#include "math/Triangle.h"
#include "objects/Mesh.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
JsonValue number(double n) {
    if (std::isnan(n))
        return "NaN";
    if (std::isinf(n))
        return n > 0 ? "Infinity" : "-Infinity";
    if (n == 0 && std::signbit(n))
        return "-0";
    return n;
}
JsonValue encode(const JsonValue& value) {
    if (std::holds_alternative<double>(value.storage()))
        return number(value.get<double>());
    if (value.isArray()) {
        JsonValue::Array result;
        for (const auto& item : value.get<JsonValue::Array>())
            result.push_back(encode(item));
        return result;
    }
    if (value.isObject()) {
        JsonValue::Object result;
        for (const auto& [key, item] : value.get<JsonValue::Object>())
            result[key] = encode(item);
        return result;
    }
    return value;
}
template <class V> JsonValue vec(const V& v) {
    JsonValue::Array result{number(v.x), number(v.y)};
    if constexpr (requires { v.z; })
        result.push_back(number(v.z));
    if constexpr (requires { v.w; })
        result.push_back(number(v.w));
    return result;
}
JsonValue plane(const Plane& p) {
    return encode(p.toJSON());
}
JsonValue line(const Line3& l) {
    return JsonValue::Array{vec(l.start), vec(l.end)};
}
JsonValue triangle(const Triangle& t) {
    return JsonValue::Array{vec(t.a), vec(t.b), vec(t.c)};
}
JsonValue ray(const Ray& r) {
    return JsonValue::Array{vec(r.origin), vec(r.direction)};
}
JsonValue box(const Box3& b) {
    return JsonValue::Array{vec(b.min), vec(b.max)};
}
JsonValue sphere(const Sphere& s) {
    return JsonValue::Array{vec(s.center), number(s.radius)};
}
JsonValue matrix(const Matrix4& m) {
    JsonValue::Array result;
    for (auto n : m.elements())
        result.push_back(number(n));
    return result;
}
JsonValue nullable(const Vector3* hit, const Vector3& target) {
    return JsonValue::Object{{"hit", hit != nullptr}, {"target", vec(target)}};
}
JsonValue::Array samples;
void emit(const char* kind, JsonValue input, JsonValue result) {
    samples.push_back(
        JsonValue::Object{{"kind", kind}, {"input", std::move(input)}, {"result", encode(result)}});
}
} // namespace

int main() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (size_t dimension : {size_t(2), size_t(3), size_t(4)}) {
        for (const auto& components : std::vector<std::vector<double>>{
                 {nan, 5, 6, 7}, {inf, 5, 6, 7}, {-0.0, 0, 0, 0},
                 {1e-200, -1e-200, 0, 0}, {1e200, -1e200, 0, 0}, {3, 4, 5, 6}}) {
            JsonValue::Array input;
            for (size_t i = 0; i < dimension; ++i) input.push_back(number(components[i]));
            JsonValue result;
            const auto check = [&](auto value) {
                return JsonValue::Object{{"normalized", vec(value.clone().normalize())},
                    {"clamped", vec(value.clone().clampLength(2, 3))},
                    {"setLength", vec(value.clone().setLength(2))}};
            };
            if (dimension == 2) result = check(Vector2(components[0], components[1]));
            else if (dimension == 3) result = check(Vector3(components[0], components[1], components[2]));
            else result = check(Vector4(components[0], components[1], components[2], components[3]));
            emit("vectorNormalization", input, result);
        }
    }
    const Matrix4 transform =
        Matrix4().set(2, .2, -.1, 3, .3, 3, .4, -2, -.2, .1, .5, 5, 0, 0, 0, 1);
    const std::vector<Vector3> points{{0, 0, 0},   {1, 2, 3},      {2, 0, 0},
                                      {.2, .3, 9}, {-1, -1, -2},   {5, 3, -4},
                                      {nan, 1, 2}, {inf, 0, -inf}, {-0.0, 0, 0}};
    const std::vector<Line3> lines{Line3({-3, 0, 0}, {4, 0, 0}),    Line3({2, 0, 0}, {2, 3, 0}),
                                   Line3({0, 0, 0}, {0, 0, 0}),     Line3({3, -1, 2}, {-1, 4, -2}),
                                   Line3({0, 0, 0}, {1e-9, 0, 0}),  Line3({nan, 0, 0}, {1, 2, 3}),
                                   Line3({-inf, 0, 0}, {inf, 0, 0})};
    const std::vector<Plane> planes{Plane({1, 0, 0}, -2),  Plane({2, -3, 4}, .5),
                                    Plane({0, 0, 0}, 0),   Plane({0, 0, 0}, 1),
                                    Plane({nan, 1, 0}, 2), Plane({inf, 0, 0}, -inf)};
    const std::vector<Box3> boxes{Box3({-1, -1, -1}, {1, 1, 1}), Box3({2, 0, 0}, {2, 0, 0}), Box3(),
                                  Box3({nan, -1, -1}, {2, 2, 2}),
                                  Box3({-inf, -inf, -inf}, {inf, inf, inf})};
    const std::vector<Sphere> spheres{Sphere({0, 0, 0}, 1), Sphere({2, 0, 0}, 0),
                                      Sphere({0, 0, 0}, -1), Sphere({nan, 0, 0}, 1),
                                      Sphere({0, 0, 0}, inf)};
    for (const auto& p : planes)
        for (size_t i = 0; i < points.size(); ++i) {
            Vector3 projected, coplanar, target(9, 8, 7), extended(9, 8, 7);
            p.projectPoint(points[i], projected);
            p.coplanarPoint(coplanar);
            const auto& l = lines[i % lines.size()];
            auto* hit = p.intersectLine(l, target);
            auto* extendedHit = p.intersectLine(l, extended, false);
            Matrix3 normalMatrix;
            normalMatrix.getNormalMatrix(transform);
            Plane restored;
            restored.fromJSON(p.toJSON());
            Plane fromPoints;
            fromPoints.setFromCoplanarPoints(l.start, l.end, points[i]);
            emit("plane",
                 JsonValue::Object{{"plane", plane(p)},
                                   {"point", vec(points[i])},
                                   {"line", line(l)},
                                   {"matrix", matrix(transform)},
                                   {"box", box(boxes[i % boxes.size()])},
                                   {"sphere", sphere(spheres[i % spheres.size()])}},
                 JsonValue::Object{
                     {"distance", number(p.distanceToPoint(points[i]))},
                     {"projected", vec(projected)},
                     {"coplanar", vec(coplanar)},
                     {"lineHit", nullable(hit, target)},
                     {"extendedHit", nullable(extendedHit, extended)},
                     {"intersectsLine", p.intersectsLine(l)},
                     {"normalized", plane(p.clone().normalize())},
                     {"negated", plane(p.clone().negate())},
                     {"translated", plane(p.clone().translate(points[i]))},
                     {"transformed", plane(p.clone().applyMatrix4(transform))},
                     {"explicitNormalMatrix",
                      plane(p.clone().applyMatrix4(transform, normalMatrix))},
                     {"restored", plane(restored)},
                     {"cloneEquals", p.clone().equals(p)},
                     {"fromPoints", plane(fromPoints)},
                     {"sphereDistance", number(p.distanceToSphere(spheres[i % spheres.size()]))},
                     {"boxHit", p.intersectsBox(boxes[i % boxes.size()])},
                     {"sphereHit", p.intersectsSphere(spheres[i % spheres.size()])}});
        }
    for (const auto& l : lines)
        for (size_t i = 0; i < lines.size(); ++i) {
            const auto& point = points[i];
            const auto& other = lines[i];
            Vector3 center, delta, along, clamped, unclamped, c1, c2;
            l.getCenter(center);
            l.delta(delta);
            l.at(.37, along);
            l.closestPointToPoint(point, true, clamped);
            l.closestPointToPoint(point, false, unclamped);
            const auto distanceSq = l.distanceSqToLine3(other, c1, c2);
            emit("line",
                 JsonValue::Object{{"line", line(l)},
                                   {"other", line(other)},
                                   {"point", vec(point)},
                                   {"matrix", matrix(transform)}},
                 JsonValue::Object{
                     {"center", vec(center)},
                     {"delta", vec(delta)},
                     {"at", vec(along)},
                     {"distance", number(l.distance())},
                     {"distanceSq", number(l.distanceSq())},
                     {"parameter", number(l.closestPointToPointParameter(point, false))},
                     {"clampedParameter", number(l.closestPointToPointParameter(point, true))},
                     {"clamped", vec(clamped)},
                     {"unclamped", vec(unclamped)},
                     {"segmentDistanceSq", number(distanceSq)},
                     {"defaultSegmentDistanceSq", number(l.distanceSqToLine3(other))},
                     {"c1", vec(c1)},
                     {"c2", vec(c2)},
                     {"transformed", line(l.clone().applyMatrix4(transform))},
                     {"cloneEquals", l.clone().equals(l)}});
        }
    const std::vector<Triangle> triangles{Triangle({0, 0, 0}, {1, 0, 0}, {0, 1, 0}),
                                          Triangle({2, -1, 3}, {-3, 2, 5}, {1, 4, -2}),
                                          Triangle(),
                                          Triangle({0, 0, 0}, {1, 0, 0}, {2, 0, 0}),
                                          Triangle({0, 0, 0}, {1e-150, 0, 0}, {0, 1e-150, 0}),
                                          Triangle({nan, 0, 0}, {1, 0, 0}, {0, 1, 0}),
                                          Triangle({inf, 0, 0}, {0, inf, 0}, {0, 0, inf})};
    for (const auto& t : triangles)
        for (size_t i = 0; i < points.size(); ++i) {
            Vector3 normal, midpoint, bary(9, 8, 7), closest, interpolation3(9, 8, 7);
            Vector2 interpolation2(9, 8);
            Vector4 interpolation4(9, 8, 7, 6);
            Plane p;
            t.getNormal(normal);
            t.getMidpoint(midpoint);
            t.getPlane(p);
            const auto baryHit = t.getBarycoord(points[i], bary);
            t.closestPointToPoint(points[i], closest);
            const auto i2 = t.getInterpolation(points[i], Vector2(1, 2), Vector2(3, -4),
                                               Vector2(5, 6), interpolation2);
            const auto i3 = t.getInterpolation(points[i], Vector3(1, 2, 3), Vector3(3, -4, 5),
                                               Vector3(5, 6, 7), interpolation3);
            const auto i4 =
                t.getInterpolation(points[i], Vector4(1, 2, 3, 4), Vector4(3, -4, 5, -6),
                                   Vector4(5, 6, 7, 8), interpolation4);
            emit("triangle",
                 JsonValue::Object{{"triangle", triangle(t)},
                                   {"point", vec(points[i])},
                                   {"box", box(boxes[i % boxes.size()])}},
                 JsonValue::Object{
                     {"area", number(t.getArea())},
                     {"normal", vec(normal)},
                     {"midpoint", vec(midpoint)},
                     {"plane", plane(p)},
                     {"bary", nullable(baryHit, bary)},
                     {"closest", vec(closest)},
                     {"contains", t.containsPoint(points[i])},
                     {"front", t.isFrontFacing(Vector3(0, 0, -1))},
                     {"back", t.isFrontFacing(Vector3(0, 0, 1))},
                     {"boxHit", t.intersectsBox(boxes[i % boxes.size()])},
                     {"interpolation2",
                      JsonValue::Object{{"hit", i2 != nullptr}, {"target", vec(interpolation2)}}},
                     {"interpolation3", nullable(i3, interpolation3)},
                     {"interpolation4",
                      JsonValue::Object{{"hit", i4 != nullptr}, {"target", vec(interpolation4)}}},
                     {"cloneEquals", t.clone().equals(t)}});
        }
    for (size_t size : {size_t(2), size_t(3), size_t(4)})
        for (bool normalized : {false, true}) {
            const std::vector<std::uint8_t> values{1, 20, 3, 40, 5, 60, 7, 80, 9, 100, 11, 120};
            Uint8BufferAttribute attribute(values, size, normalized);
            const Vector3 bary(.2, .3, .5);
            Vector2 v2;
            Vector3 v3;
            Vector4 v4;
            Triangle::getInterpolatedAttribute(attribute, 0, 1, 2, bary, v2);
            Triangle::getInterpolatedAttribute(attribute, 0, 1, 2, bary, v3);
            Triangle::getInterpolatedAttribute(attribute, 0, 1, 2, bary, v4);
            JsonValue::Array data;
            for (auto n : values)
                data.push_back(n);
            emit("attribute",
                 JsonValue::Object{{"values", data},
                                   {"itemSize", size},
                                   {"normalized", normalized},
                                   {"bary", vec(bary)},
                                   {"indices", JsonValue::Array{0, 1, 2}}},
                 JsonValue::Object{{"v2", vec(v2)}, {"v3", vec(v3)}, {"v4", vec(v4)}});
        }
    {
        Float32BufferAttribute attribute(std::vector<float>{1, 2, 3, 4, 5, 6, 7, 8, 9}, 3);
        const Vector3 bary(.2, .3, .5);
        Vector3 v3;
        Vector4 v4;
        Triangle::getInterpolatedAttribute(attribute, 0, 1, 2, bary, v3);
        Triangle::getInterpolatedAttribute(attribute, 0, 1, 2, bary, v4);
        Triangle t;
        t.setFromAttributeAndIndices(attribute, 2, 0, 1);
        std::vector<Vector3> data{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
        Triangle indexed;
        indexed.setFromPointsAndIndices(data, 2, 0, 1);
        emit("attribute3Tail", JsonValue::Object{},
             JsonValue::Object{{"v3", vec(v3)},
                               {"v4", vec(v4)},
                               {"attributeTriangle", triangle(t)},
                               {"indexedTriangle", triangle(indexed)}});
    }
    const std::vector<Ray> rays{Ray(),
                                Ray({0, 0, 0}, {1, 0, 0}),
                                Ray({2, 0, 0}, {-1, 0, 0}),
                                Ray({.2, .3, 2}, {0, 0, -1}),
                                Ray({.2, .3, -2}, {0, 0, 1}),
                                Ray({1, 1, 1}, {0, 0, 0}),
                                Ray({1, 0, 0}, {-0.0, 1, 0}),
                                Ray({0, 0, 0}, Vector3(1, 2, 3).normalize()),
                                Ray({0, 0, 0}, {2, 0, 0}),
                                Ray({nan, 0, 0}, {0, 0, -1}),
                                Ray({0, 0, 0}, {inf, 0, -1})};
    for (const auto& r : rays) {
        for (const auto& s : spheres) {
            Vector3 target(9, 8, 7);
            const auto* hit = r.intersectSphere(s, target);
            emit("raySphere", JsonValue::Object{{"ray", ray(r)}, {"sphere", sphere(s)}},
                 JsonValue::Object{{"intersection", nullable(hit, target)},
                                   {"intersects", r.intersectsSphere(s)}});
        }
        for (const auto& p : planes) {
            Vector3 target(9, 8, 7);
            const auto* hit = r.intersectPlane(p, target);
            const auto distance = r.distanceToPlane(p);
            emit(
                "rayPlane", JsonValue::Object{{"ray", ray(r)}, {"plane", plane(p)}},
                JsonValue::Object{{"intersection", nullable(hit, target)},
                                  {"intersects", r.intersectsPlane(p)},
                                  {"distance", distance ? number(*distance) : JsonValue(nullptr)}});
        }
        for (const auto& b : boxes) {
            Vector3 target(9, 8, 7);
            const auto* hit = r.intersectBox(b, target);
            emit("rayBox", JsonValue::Object{{"ray", ray(r)}, {"box", box(b)}},
                 JsonValue::Object{{"intersection", nullable(hit, target)},
                                   {"intersects", r.intersectsBox(b)}});
        }
        for (const auto& t : triangles)
            for (bool cull : {false, true}) {
                Vector3 target(9, 8, 7);
                const auto* hit = r.intersectTriangle(t.a, t.b, t.c, cull, target);
                emit("rayTriangle",
                     JsonValue::Object{{"ray", ray(r)}, {"triangle", triangle(t)}, {"cull", cull}},
                     nullable(hit, target));
            }
        for (const auto& p : points) {
            Vector3 target;
            r.closestPointToPoint(p, target);
            emit("rayPoint", JsonValue::Object{{"ray", ray(r)}, {"point", vec(p)}},
                 JsonValue::Object{{"closest", vec(target)},
                                   {"distance", number(r.distanceToPoint(p))},
                                   {"distanceSq", number(r.distanceSqToPoint(p))}});
        }
        for (const auto& l : lines) {
            Vector3 rayPoint, segmentPoint;
            const auto distance = r.distanceSqToSegment(l.start, l.end, &rayPoint, &segmentPoint);
            emit("raySegment", JsonValue::Object{{"ray", ray(r)}, {"line", line(l)}},
                 JsonValue::Object{
                     {"distanceSq", number(distance)},
                     {"rayPoint", vec(rayPoint)},
                     {"segmentPoint", vec(segmentPoint)},
                     {"defaultDistanceSq", number(r.distanceSqToSegment(l.start, l.end))}});
        }
        Vector3 at;
        r.at(.37, at);
        emit("rayMutators", JsonValue::Object{{"ray", ray(r)}, {"matrix", matrix(transform)}},
             JsonValue::Object{{"at", vec(at)},
                               {"recast", ray(r.clone().recast(.37))},
                               {"lookAt", ray(r.clone().lookAt(Vector3(4, 5, 6)))},
                               {"transformed", ray(r.clone().applyMatrix4(transform))},
                               {"cloneEquals", r.clone().equals(r)}});
    }
    for (bool ortho : {false, true})
        for (int coordinate : {WebGLCoordinateSystem, WebGPUCoordinateSystem})
            for (bool reverse : {false, true}) {
                Matrix4 projection;
                if (ortho)
                    projection.makeOrthographic(-2, 3, 4, -1, .3, 20, coordinate, reverse);
                else
                    projection.makePerspective(-.4, .7, .5, -.2, .3, 20, coordinate, reverse);
                Frustum f;
                f.setFromProjectionMatrix(projection, coordinate, reverse);
                JsonValue::Array fplanes, contained, sphereHits, boxHits, spriteHits;
                for (const auto& p : f.planes)
                    fplanes.push_back(plane(p));
                const std::vector<Vector3> fp{{0, 0, -.3},     {0, 0, -20}, {0, 0, -.299},
                                              {0, 0, -20.001}, {0, 0, -2},  {3, 4, -2},
                                              {-2, -1, -2},    {nan, 0, 0}};
                for (const auto& p : fp)
                    contained.push_back(f.containsPoint(p));
                for (const auto& s : spheres)
                    sphereHits.push_back(f.intersectsSphere(s));
                for (const auto& b : boxes)
                    boxHits.push_back(f.intersectsBox(b));
                Matrix4 world;
                world.makeTranslation(0, 0, -2);
                for (const auto& center : {Vector2(.5, .5), Vector2(0, 1), Vector2(5, 3)})
                    spriteHits.push_back(f.intersectsSprite(center, world));
                Mesh mesh(std::make_shared<BoxGeometry>(1, 2, 3));
                mesh.matrixWorld.copy(world);
                const bool objectHit = f.intersectsObject(mesh);
                struct BoundedObject {
                    std::optional<Sphere> boundingSphere;
                    Matrix4 matrixWorld;
                    int computes = 0;
                    void computeBoundingSphere() {
                        boundingSphere = Sphere(Vector3(), 1);
                        ++computes;
                    }
                } owned;
                owned.matrixWorld.copy(world);
                const bool ownedHit = f.intersectsObject(owned);
                const auto clone = f.clone();
                bool cloneEquals = true;
                for (size_t i = 0; i < 6; ++i)
                    cloneEquals = cloneEquals && clone.planes[i].equals(f.planes[i]);
                emit("frustum",
                     JsonValue::Object{{"matrix", matrix(projection)},
                                       {"coordinate", coordinate},
                                       {"reverse", reverse}},
                     JsonValue::Object{
                         {"planes", fplanes},
                         {"contained", contained},
                         {"sphereHits", sphereHits},
                         {"boxHits", boxHits},
                         {"spriteHits", spriteHits},
                         {"objectHit", objectHit},
                         {"geometryComputed", mesh.geometry->boundingSphere.has_value()},
                         {"ownedHit", ownedHit},
                         {"ownedComputes", owned.computes},
                         {"cloneEquals", cloneEquals}});
            }
    {
        Matrix4 zero;
        zero.set(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        Frustum f;
        f.setFromProjectionMatrix(zero);
        JsonValue::Array fplanes;
        for (const auto& p : f.planes)
            fplanes.push_back(plane(p));
        bool invalidThrows = false;
        try {
            f.setFromProjectionMatrix(Matrix4(), -1);
        } catch (const std::invalid_argument&) {
            invalidThrows = true;
        }
        f.setFromProjectionMatrix(Matrix4(), -1, true);
        emit("frustumEdge", JsonValue::Object{},
             JsonValue::Object{{"zeroPlanes", fplanes},
                               {"invalidThrows", invalidThrows},
                               {"invalidReversedContained", f.containsPoint(Vector3())}});
    }
    {
        Line3 l;
        l.set(Vector3(1, 2, 3), Vector3(5, 6, 7));
        l.at(.37, l.start);
        Ray r;
        r.set(Vector3(1, 2, 3), Vector3(0, 0, -1));
        r.at(.37, r.direction);
        Plane p;
        p.setComponents(1, 0, 0, -2);
        p.projectPoint(Vector3(1, 2, 3), p.normal);
        Plane n;
        n.setFromNormalAndCoplanarPoint(Vector3(0, 2, 0), Vector3(3, 4, 5));
        Triangle t;
        t.set(Vector3(2, 3, 4), Vector3(5, 6, 7), Vector3(8, 10, 9));
        t.getNormal(t.a);
        Float32BufferAttribute attr(std::vector<float>{1, 2, 3, 4, 5, 6, 7, 8, 9}, 3);
        Vector3 bary(.2, .3, .5);
        Triangle::getInterpolatedAttribute(attr, 0, 1, 2, bary, bary);
        Frustum f;
        f.set(Plane({1, 0, 0}, 1), Plane({-1, 0, 0}, 1), Plane({0, 1, 0}, 1), Plane({0, -1, 0}, 1),
              Plane({0, 0, 1}, 1), Plane({0, 0, -1}, 1));
        emit("aliases", JsonValue::Object{},
             JsonValue::Object{{"lineAtStart", line(l)},
                               {"rayAtDirection", ray(r)},
                               {"planeProjectNormal", plane(p)},
                               {"planeFromNormal", plane(n)},
                               {"triangleNormalA", triangle(t)},
                               {"attributeBaryTarget", vec(bary)},
                               {"frustumInside", f.containsPoint(Vector3(1, 1, 1))},
                               {"frustumOutside", f.containsPoint(Vector3(1.0000001, 0, 0))}});
    }
    std::cout << JsonValue(samples).dump() << '\n';
}
