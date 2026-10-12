#include "geometries/SphereGeometry.h"
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
SphereGeometry::SphereGeometry(double radius, double widthSegments, double heightSegments, double phiStart, double phiLength, double thetaStart, double thetaLength)
    : parameters{radius, widthSegments, heightSegments, phiStart, phiLength, thetaStart, thetaLength} {
    type = "SphereGeometry";
    widthSegments = MathUtils::max(3, std::floor(widthSegments));
    heightSegments = MathUtils::max(2, std::floor(heightSegments));
    requireTerminating(widthSegments);
    requireTerminating(heightSegments);
    const double thetaEnd = MathUtils::min(thetaStart + thetaLength, pi);
    double index = 0;
    std::vector<std::vector<double>> grid;
    std::vector<double> indices, vertices, normals, uvs;
    Vector3 vertex, normal;
    for (double iy = 0; iy <= heightSegments; ++iy) {
        std::vector<double> row;
        const double v = iy / heightSegments;
        const double theta = thetaStart + v * thetaLength;
        const double y = radius * std::cos(theta);
        const double ringRadius = std::sqrt(radius * radius - y * y);
        double uOffset = 0;
        if (iy == 0 && thetaStart == 0) uOffset = 0.5 / widthSegments;
        else if (iy == heightSegments && thetaEnd == pi) uOffset = -0.5 / widthSegments;
        for (double ix = 0; ix <= widthSegments; ++ix) {
            const double u = ix / widthSegments;
            const double phi = phiStart + u * phiLength;
            vertex.set(-ringRadius * std::cos(phi), y, ringRadius * std::sin(phi));
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normal.copy(vertex).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {u + uOffset, 1 - v});
            row.push_back(index++);
        }
        grid.push_back(std::move(row));
    }
    for (std::size_t iy = 0; double(iy) < heightSegments; ++iy) {
        for (std::size_t ix = 0; double(ix) < widthSegments; ++ix) {
            const double a = grid[iy][ix + 1], b = grid[iy][ix],
                         c = grid[iy + 1][ix], d = grid[iy + 1][ix + 1];
            if (iy != 0 || thetaStart > 0) indices.insert(indices.end(), {a, b, d});
            if (double(iy) != heightSegments - 1 || thetaEnd < pi) indices.insert(indices.end(), {b, c, d});
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
SphereGeometry& SphereGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const SphereGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> SphereGeometry::clone() const {
    auto geometry = std::make_shared<SphereGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue SphereGeometry::toJSON() const {
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
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["phiStart"] = parameters.phiStart;
    result["phiLength"] = parameters.phiLength;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<SphereGeometry> SphereGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<SphereGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "widthSegments", 32), jsonNumber(data, "heightSegments", 16), jsonNumber(data, "phiStart", 0), jsonNumber(data, "phiLength", std::numbers::pi * 2), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi));
}
