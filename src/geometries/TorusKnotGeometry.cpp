#include "geometries/TorusKnotGeometry.h"
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
TorusKnotGeometry::TorusKnotGeometry(double radius, double tube, double tubularSegments, double radialSegments, double p, double q)
    : parameters{radius, tube, tubularSegments, radialSegments, p, q} {
    type = "TorusKnotGeometry";
    tubularSegments = std::floor(tubularSegments);
    radialSegments = std::floor(radialSegments);
    requireTerminating(tubularSegments);
    requireTerminating(radialSegments);
    std::vector<double> indices, vertices, normals, uvs;
    Vector3 vertex, normal, P1, P2, B, T, N;
    const auto calculatePositionOnCurve = [&](double u, Vector3& position) {
        const double cu = JSNumberMath::cos(u), su = JSNumberMath::sin(u);
        const double quOverP = q / p * u;
        const double cs = JSNumberMath::cos(quOverP);
        position.x = radius * (2 + cs) * 0.5 * cu;
        position.y = radius * (2 + cs) * su * 0.5;
        position.z = radius * JSNumberMath::sin(quOverP) * 0.5;
    };
    for (double i = 0; i <= tubularSegments; ++i) {
        const double u = i / tubularSegments * p * pi * 2;
        calculatePositionOnCurve(u, P1);
        calculatePositionOnCurve(u + 0.01, P2);
        T.subVectors(P2, P1);
        N.addVectors(P2, P1);
        B.crossVectors(T, N);
        N.crossVectors(B, T);
        B.normalize();
        N.normalize();
        for (double j = 0; j <= radialSegments; ++j) {
            const double v = j / radialSegments * pi * 2;
            const double cx = -tube * JSNumberMath::cos(v), cy = tube * JSNumberMath::sin(v);
            vertex.x = P1.x + (cx * N.x + cy * B.x);
            vertex.y = P1.y + (cx * N.y + cy * B.y);
            vertex.z = P1.z + (cx * N.z + cy * B.z);
            vertices.insert(vertices.end(), {vertex.x, vertex.y, vertex.z});
            normal.subVectors(vertex, P1).normalize();
            normals.insert(normals.end(), {normal.x, normal.y, normal.z});
            uvs.insert(uvs.end(), {i / tubularSegments, j / radialSegments});
        }
    }
    for (double j = 1; j <= tubularSegments; ++j) {
        for (double i = 1; i <= radialSegments; ++i) {
            const double a = (radialSegments + 1) * (j - 1) + (i - 1),
                         b = (radialSegments + 1) * j + (i - 1),
                         c = (radialSegments + 1) * j + i,
                         d = (radialSegments + 1) * (j - 1) + i;
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    }
    finish(*this, indices, vertices, normals, uvs);
}
TorusKnotGeometry& TorusKnotGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const TorusKnotGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> TorusKnotGeometry::clone() const {
    auto geometry = std::make_shared<TorusKnotGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue TorusKnotGeometry::toJSON() const {
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
    result["tubularSegments"] = parameters.tubularSegments;
    result["radialSegments"] = parameters.radialSegments;
    result["p"] = parameters.p;
    result["q"] = parameters.q;
    return result;
}
std::shared_ptr<TorusKnotGeometry> TorusKnotGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<TorusKnotGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "tube", 0.4), jsonNumber(data, "tubularSegments", 64), jsonNumber(data, "radialSegments", 8), jsonNumber(data, "p", 2), jsonNumber(data, "q", 3));
}
