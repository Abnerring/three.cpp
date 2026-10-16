#include "geometries/CircleGeometry.h"
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
CircleGeometry::CircleGeometry(double radius, double segments, double thetaStart, double thetaLength)
    : parameters{radius, segments, thetaStart, thetaLength} {
    type = "CircleGeometry";
    segments = MathUtils::max(3, segments);
    requireTerminating(segments);
    std::vector<double> indices, vertices{0, 0, 0}, normals{0, 0, 1}, uvs{0.5, 0.5};
    Vector3 vertex;
    for (double s = 0; s <= segments; ++s) {
        const double segment = thetaStart + s / segments * thetaLength;
        vertex.x = radius * JSNumberMath::cos(segment);
        vertex.y = radius * JSNumberMath::sin(segment);
        vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
        normals.insert(normals.end(), {0, 0, 1});
        uvs.insert(uvs.end(), {(vertex.x / radius + 1) / 2, (vertex.y / radius + 1) / 2});
    }
    for (double i = 1; i <= segments; ++i)
        indices.insert(indices.end(), {i, i + 1, 0});
    finish(*this, indices, vertices, normals, uvs);
}
CircleGeometry& CircleGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const CircleGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> CircleGeometry::clone() const {
    auto geometry = std::make_shared<CircleGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue CircleGeometry::toJSON() const {
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
    result["radius"] = parameters.radius;
    result["segments"] = parameters.segments;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<CircleGeometry> CircleGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<CircleGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "segments", 32), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}
