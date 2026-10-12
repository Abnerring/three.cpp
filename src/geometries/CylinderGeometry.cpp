#include "geometries/CylinderGeometry.h"
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
bool jsonBoolean(const JsonValue& data, const char* key, bool fallback) {
    return data.contains(key) ? JSValue::fromJSON(data.at(key)).toBoolean() : fallback;
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
CylinderGeometry::CylinderGeometry(double radiusTop, double radiusBottom, double height, double radialSegments, double heightSegments, bool openEnded, double thetaStart, double thetaLength)
    : parameters{radiusTop, radiusBottom, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength} {
    type = "CylinderGeometry";
    radialSegments = std::floor(radialSegments);
    heightSegments = std::floor(heightSegments);
    requireTerminating(radialSegments);
    requireTerminating(heightSegments);
    std::vector<double> indices, vertices, normals, uvs;
    double index = 0;
    std::vector<std::vector<double>> indexArray;
    const double halfHeight = height / 2;
    std::size_t groupStart = 0, groupCount = 0;
    const double slope = (radiusBottom - radiusTop) / height;
    Vector3 normal, vertex;
    for (double y = 0; y <= heightSegments; ++y) {
        std::vector<double> row;
        const double v = y / heightSegments;
        const double radius = v * (radiusBottom - radiusTop) + radiusTop;
        for (double x = 0; x <= radialSegments; ++x) {
            const double u = x / radialSegments;
            const double theta = u * thetaLength + thetaStart;
            const double sinTheta = std::sin(theta), cosTheta = std::cos(theta);
            vertex.set(radius * sinTheta, -v * height + halfHeight, radius * cosTheta);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normal.set(sinTheta, slope, cosTheta).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {u, 1 - v});
            row.push_back(index++);
        }
        indexArray.push_back(std::move(row));
    }
    for (std::size_t x = 0; double(x) < radialSegments; ++x) {
        for (std::size_t y = 0; double(y) < heightSegments; ++y) {
            const double a = indexArray[y][x], b = indexArray[y + 1][x],
                         c = indexArray[y + 1][x + 1], d = indexArray[y][x + 1];
            if (radiusTop > 0 || y != 0) {
                indices.insert(indices.end(), {a, b, d});
                groupCount += 3;
            }
            if (radiusBottom > 0 || double(y) != heightSegments - 1) {
                indices.insert(indices.end(), {b, c, d});
                groupCount += 3;
            }
        }
    }
    addGroup(groupStart, groupCount, 0);
    groupStart += groupCount;
    const auto generateCap = [&](bool top) {
        const double centerIndexStart = index;
        std::size_t capCount = 0;
        const double radius = top ? radiusTop : radiusBottom;
        const double sign = top ? 1 : -1;
        for (double x = 1; x <= radialSegments; ++x) {
            vertices.insert(vertices.end(), {0, halfHeight * sign, 0});
            normals.insert(normals.end(), {0, sign, 0});
            uvs.insert(uvs.end(), {0.5, 0.5});
            ++index;
        }
        const double centerIndexEnd = index;
        for (double x = 0; x <= radialSegments; ++x) {
            const double u = x / radialSegments;
            const double theta = u * thetaLength + thetaStart;
            const double cosTheta = std::cos(theta), sinTheta = std::sin(theta);
            vertex.set(radius * sinTheta, halfHeight * sign, radius * cosTheta);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normals.insert(normals.end(), {0, sign, 0});
            uvs.insert(uvs.end(), {cosTheta * 0.5 + 0.5, sinTheta * 0.5 * sign + 0.5});
            ++index;
        }
        for (double x = 0; x < radialSegments; ++x) {
            const double c = centerIndexStart + x, i = centerIndexEnd + x;
            if (top) indices.insert(indices.end(), {i, i + 1, c});
            else indices.insert(indices.end(), {i + 1, i, c});
            capCount += 3;
        }
        addGroup(groupStart, capCount, top ? 1 : 2);
        groupStart += capCount;
    };
    if (!openEnded) {
        if (radiusTop > 0) generateCap(true);
        if (radiusBottom > 0) generateCap(false);
    }
    finish(*this, indices, vertices, normals, uvs);
}
CylinderGeometry& CylinderGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const CylinderGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> CylinderGeometry::clone() const {
    auto geometry = std::make_shared<CylinderGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue CylinderGeometry::toJSON() const {
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
    result["radiusTop"] = parameters.radiusTop;
    result["radiusBottom"] = parameters.radiusBottom;
    result["height"] = parameters.height;
    result["radialSegments"] = parameters.radialSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["openEnded"] = parameters.openEnded;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<CylinderGeometry> CylinderGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<CylinderGeometry>(jsonNumber(data, "radiusTop", 1), jsonNumber(data, "radiusBottom", 1), jsonNumber(data, "height", 1), jsonNumber(data, "radialSegments", 32), jsonNumber(data, "heightSegments", 1), jsonBoolean(data, "openEnded", false), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}
