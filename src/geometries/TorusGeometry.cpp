#include "geometries/TorusGeometry.h"
#include "math/MathUtils.h"
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
TorusGeometry::TorusGeometry(double radius, double tube, double radialSegments, double tubularSegments, double arc, double thetaStart, double thetaLength)
    : parameters{radius, tube, radialSegments, tubularSegments, arc, thetaStart, thetaLength} {
    type = "TorusGeometry";
    radialSegments = std::floor(radialSegments);
    tubularSegments = std::floor(tubularSegments);
    requireTerminating(radialSegments);
    requireTerminating(tubularSegments);
    std::vector<double> indices, vertices, normals, uvs;
    Vector3 center, vertex, normal;
    for (double j = 0; j <= radialSegments; ++j) {
        const double v = thetaStart + (j / radialSegments) * thetaLength;
        for (double i = 0; i <= tubularSegments; ++i) {
            const double u = i / tubularSegments * arc;
            vertex.set((radius + tube * std::cos(v)) * std::cos(u),
                       (radius + tube * std::cos(v)) * std::sin(u), tube * std::sin(v));
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            center.x = radius * std::cos(u);
            center.y = radius * std::sin(u);
            normal.subVectors(vertex, center).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {i / tubularSegments, j / radialSegments});
        }
    }
    for (double j = 1; j <= radialSegments; ++j) {
        for (double i = 1; i <= tubularSegments; ++i) {
            const double a = (tubularSegments + 1) * j + i - 1,
                         b = (tubularSegments + 1) * (j - 1) + i - 1,
                         c = (tubularSegments + 1) * (j - 1) + i,
                         d = (tubularSegments + 1) * j + i;
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
TorusGeometry& TorusGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const TorusGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> TorusGeometry::clone() const {
    auto geometry = std::make_shared<TorusGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue TorusGeometry::toJSON() const {
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
    result["tube"] = parameters.tube;
    result["radialSegments"] = parameters.radialSegments;
    result["tubularSegments"] = parameters.tubularSegments;
    result["arc"] = parameters.arc;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<TorusGeometry> TorusGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<TorusGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "tube", 0.4), jsonNumber(data, "radialSegments", 12), jsonNumber(data, "tubularSegments", 48), jsonNumber(data, "arc", std::numbers::pi * 2), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}
