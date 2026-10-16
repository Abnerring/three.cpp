#include "geometries/LatheGeometry.h"
#include "math/JSNumberMath.h"
#include "math/MathUtils.h"
#include "math/JSNumberMath.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr double pi = std::numbers::pi;
void requireTerminating(double segments) {
    if (segments == std::numeric_limits<double>::infinity())
        throw JSRangeError("Infinite geometry subdivisions do not terminate");
}
double jsonNumber(const JsonValue& data, const char* key, double fallback) {
    return data.contains(key) ? JSValue::fromJSON(data.at(key)).toNumber() : fallback;
}

void finish(BufferGeometry& geometry, const std::vector<double>& indices,
            const std::vector<double>& vertices, const std::vector<double>& normals,
            const std::vector<double>& uvs, bool uvBeforeNormal = false) {
    if (std::any_of(indices.begin(), indices.end(), [](double i) { return i >= 65535; }))
        geometry.setIndex(std::make_shared<Uint32BufferAttribute>(indices, 1));
    else
        geometry.setIndex(std::make_shared<Uint16BufferAttribute>(indices, 1));
    geometry.setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    if (uvBeforeNormal)
        geometry.setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
    geometry.setAttribute("normal", std::make_shared<Float32BufferAttribute>(normals, 3));
    if (!uvBeforeNormal)
        geometry.setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
}
} // namespace
LatheGeometry::LatheGeometry(PointsPtr points, double segments, double phiStart, double phiLength)
    : parameters{points, segments, phiStart, phiLength} {
    type = "LatheGeometry";
    if (!points) throw JSTypeError("LatheGeometry points cannot be null");
    segments = std::floor(segments);
    requireTerminating(segments);
    phiLength = MathUtils::clamp(phiLength, 0, pi * 2);
    std::vector<double> indices, vertices, normals, uvs, initNormals;
    const double inverseSegments = 1.0 / segments;
    Vector3 vertex, normal, curNormal, prevNormal;
    const auto point = [&](std::size_t j) -> const Vector2& {
        if (j >= points->size() || !(*points)[j])
            throw JSTypeError("LatheGeometry requires an existing profile point");
        return *(*points)[j];
    };
    double dx = 0, dy = 0;
    for (std::size_t j = 0; j < points->size(); ++j) {
        if (j == 0) {
            dx = point(j + 1).x - point(j).x;
            dy = point(j + 1).y - point(j).y;
            normal.set(dy * 1.0, -dx, dy * 0.0);
            prevNormal.copy(normal);
            normal.normalize();
            initNormals.insert(initNormals.end(), {normal.x, normal.y, normal.z});
        } else if (j == points->size() - 1) {
            initNormals.insert(initNormals.end(), {prevNormal.x, prevNormal.y, prevNormal.z});
        } else {
            dx = point(j + 1).x - point(j).x;
            dy = point(j + 1).y - point(j).y;
            normal.set(dy * 1.0, -dx, dy * 0.0);
            curNormal.copy(normal);
            normal.x += prevNormal.x;
            normal.y += prevNormal.y;
            normal.z += prevNormal.z;
            normal.normalize();
            initNormals.insert(initNormals.end(), {normal.x, normal.y, normal.z});
            prevNormal.copy(curNormal);
        }
    }
    for (double i = 0; i <= segments; ++i) {
        const double phi = phiStart + i * inverseSegments * phiLength;
        const double sin = JSNumberMath::sin(phi), cos = JSNumberMath::cos(phi);
        for (std::size_t j = 0; j < points->size(); ++j) {
            vertex.set(point(j).x * sin, point(j).y, point(j).x * cos);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            uvs.insert(uvs.end(), {i / segments, double(j) / double(points->size() - 1)});
            normals.insert(normals.end(), {initNormals[3 * j] * sin,
                initNormals[3 * j + 1], initNormals[3 * j] * cos});
        }
    }
    for (double i = 0; i < segments; ++i) {
        for (std::size_t j = 0; j + 1 < points->size(); ++j) {
            const double base = double(j) + i * double(points->size());
            const double a = base, b = base + double(points->size()),
                         c = base + double(points->size()) + 1, d = base + 1;
            indices.insert(indices.end(), {a, b, d, c, d, b});
        }
    }
    finish(*this, indices, vertices, normals, uvs, true);
}
LatheGeometry& LatheGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const LatheGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> LatheGeometry::clone() const {
    auto geometry = std::make_shared<LatheGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue LatheGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON();
        result["type"] = "BufferGeometry";
        return result;
    }
    // Parameterized geometries return before visiting vertex storage upstream.
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    JsonValue::Array points;
    for (const auto& point : *parameters.points)
        points.emplace_back(point ? JsonValue(JsonValue::Object{{"x", point->x}, {"y", point->y}}) : JsonValue(nullptr));
    result["points"] = std::move(points);
    result["segments"] = parameters.segments;
    result["phiStart"] = parameters.phiStart;
    result["phiLength"] = parameters.phiLength;
    return result;
}
std::shared_ptr<LatheGeometry> LatheGeometry::fromJSON(const JsonValue& data) {
    auto points = defaultPoints();
    if (data.contains("points")) {
        if (!data.at("points").isArray()) throw JSTypeError("LatheGeometry JSON points must be an array");
        points = std::make_shared<Points>();
        for (const auto& point : data.at("points").get<JsonValue::Array>()) {
            if (point.isNull()) points->push_back(nullptr);
            else points->push_back(std::make_shared<Vector2>(
                jsonNumber(point, "x", std::numeric_limits<double>::quiet_NaN()),
                jsonNumber(point, "y", std::numeric_limits<double>::quiet_NaN())));
        }
    }
    return std::make_shared<LatheGeometry>(points, jsonNumber(data, "segments", 12),
        jsonNumber(data, "phiStart", 0), jsonNumber(data, "phiLength", pi * 2));
}
LatheGeometry::PointsPtr LatheGeometry::defaultPoints() {
    return std::make_shared<Points>(Points{
        std::make_shared<Vector2>(0, -0.5), std::make_shared<Vector2>(0.5, 0),
        std::make_shared<Vector2>(0, 0.5)});
}
