#include "geometries/RingGeometry.h"
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
RingGeometry::RingGeometry(double innerRadius, double outerRadius, double thetaSegments, double phiSegments, double thetaStart, double thetaLength)
    : parameters{innerRadius, outerRadius, thetaSegments, phiSegments, thetaStart, thetaLength} {
    type = "RingGeometry";
    thetaSegments = MathUtils::max(3, thetaSegments);
    phiSegments = MathUtils::max(1, phiSegments);
    requireTerminating(thetaSegments);
    requireTerminating(phiSegments);
    std::vector<double> indices, vertices, normals, uvs;
    double radius = innerRadius;
    const double radiusStep = (outerRadius - innerRadius) / phiSegments;
    Vector3 vertex;
    for (double j = 0; j <= phiSegments; ++j) {
        for (double i = 0; i <= thetaSegments; ++i) {
            const double segment = thetaStart + i / thetaSegments * thetaLength;
            vertex.x = radius * JSNumberMath::cos(segment);
            vertex.y = radius * JSNumberMath::sin(segment);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normals.insert(normals.end(), {0, 0, 1});
            uvs.insert(uvs.end(), {(vertex.x / outerRadius + 1) / 2, (vertex.y / outerRadius + 1) / 2});
        }
        radius += radiusStep;
    }
    for (double j = 0; j < phiSegments; ++j) {
        const double thetaSegmentLevel = j * (thetaSegments + 1);
        for (double i = 0; i < thetaSegments; ++i) {
            const double segment = i + thetaSegmentLevel;
            const double a = segment, b = segment + thetaSegments + 1,
                         c = segment + thetaSegments + 2, d = segment + 1;
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
RingGeometry& RingGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const RingGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> RingGeometry::clone() const {
    auto geometry = std::make_shared<RingGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue RingGeometry::toJSON() const {
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
    result["innerRadius"] = parameters.innerRadius;
    result["outerRadius"] = parameters.outerRadius;
    result["thetaSegments"] = parameters.thetaSegments;
    result["phiSegments"] = parameters.phiSegments;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<RingGeometry> RingGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<RingGeometry>(jsonNumber(data, "innerRadius", 0.5), jsonNumber(data, "outerRadius", 1), jsonNumber(data, "thetaSegments", 32), jsonNumber(data, "phiSegments", 1), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}
