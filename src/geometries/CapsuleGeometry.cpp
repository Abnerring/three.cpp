#include "geometries/CapsuleGeometry.h"
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
CapsuleGeometry::CapsuleGeometry(double radius, double height, double capSegments, double radialSegments, double heightSegments)
    : parameters{radius, height, capSegments, radialSegments, heightSegments} {
    type = "CapsuleGeometry";
    height = MathUtils::max(0, height);
    capSegments = MathUtils::max(1, std::floor(capSegments));
    radialSegments = MathUtils::max(3, std::floor(radialSegments));
    heightSegments = MathUtils::max(1, std::floor(heightSegments));
    requireTerminating(capSegments);
    requireTerminating(radialSegments);
    requireTerminating(heightSegments);
    std::vector<double> indices, vertices, normals, uvs;
    const double halfHeight = height / 2;
    const double capArcLength = (pi / 2) * radius;
    const double cylinderPartLength = height;
    const double totalArcLength = 2 * capArcLength + cylinderPartLength;
    const double numVerticalSegments = capSegments * 2 + heightSegments;
    const double verticesPerRow = radialSegments + 1;
    Vector3 normal, vertex;
    for (double iy = 0; iy <= numVerticalSegments; ++iy) {
        double currentArcLength = 0, profileY = 0, profileRadius = 0, normalYComponent = 0;
        if (iy <= capSegments) {
            const double progress = iy / capSegments;
            const double angle = (progress * pi) / 2;
            profileY = -halfHeight - radius * std::cos(angle);
            profileRadius = radius * std::sin(angle);
            normalYComponent = -radius * std::cos(angle);
            currentArcLength = progress * capArcLength;
        } else if (iy <= capSegments + heightSegments) {
            const double progress = (iy - capSegments) / heightSegments;
            profileY = -halfHeight + progress * height;
            profileRadius = radius;
            normalYComponent = 0;
            currentArcLength = capArcLength + progress * cylinderPartLength;
        } else {
            const double progress = (iy - capSegments - heightSegments) / capSegments;
            const double angle = (progress * pi) / 2;
            profileY = halfHeight + radius * std::sin(angle);
            profileRadius = radius * std::cos(angle);
            normalYComponent = radius * std::sin(angle);
            currentArcLength = capArcLength + cylinderPartLength + progress * capArcLength;
        }
        const double v = MathUtils::max(0, MathUtils::min(1, currentArcLength / totalArcLength));
        double uOffset = 0;
        if (iy == 0) uOffset = 0.5 / radialSegments;
        else if (iy == numVerticalSegments) uOffset = -0.5 / radialSegments;
        for (double ix = 0; ix <= radialSegments; ++ix) {
            const double u = ix / radialSegments;
            const double theta = u * pi * 2;
            const double sinTheta = std::sin(theta), cosTheta = std::cos(theta);
            vertex.set(-profileRadius * cosTheta, profileY, profileRadius * sinTheta);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normal.set(-profileRadius * cosTheta, normalYComponent, profileRadius * sinTheta).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {u + uOffset, v});
        }
        if (iy > 0) {
            const double prevIndexRow = (iy - 1) * verticesPerRow;
            for (double ix = 0; ix < radialSegments; ++ix) {
                const double i1 = prevIndexRow + ix, i2 = prevIndexRow + ix + 1,
                             i3 = iy * verticesPerRow + ix, i4 = iy * verticesPerRow + ix + 1;
                indices.insert(indices.end(), {i1, i2, i3, i2, i4, i3});
            }
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
CapsuleGeometry& CapsuleGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const CapsuleGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> CapsuleGeometry::clone() const {
    auto geometry = std::make_shared<CapsuleGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue CapsuleGeometry::toJSON() const {
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
    result["height"] = parameters.height;
    result["capSegments"] = parameters.capSegments;
    result["radialSegments"] = parameters.radialSegments;
    result["heightSegments"] = parameters.heightSegments;
    return result;
}
std::shared_ptr<CapsuleGeometry> CapsuleGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<CapsuleGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "height", 1), jsonNumber(data, "capSegments", 4), jsonNumber(data, "radialSegments", 8), jsonNumber(data, "heightSegments", 1));
}
