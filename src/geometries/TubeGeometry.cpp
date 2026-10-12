#include "geometries/TubeGeometry.h"
#include "extras/curves/QuadraticBezierCurve3.h"
#include "extras/curves/LineCurve3.h"
#include "extras/curves/CubicBezierCurve3.h"
#include "extras/curves/CatmullRomCurve3.h"
#include <numbers>

std::shared_ptr<Curve<Vector3>> TubeGeometry::defaultPath() {
    return std::make_shared<QuadraticBezierCurve3>(std::make_shared<Vector3>(-1, -1, 0),
        std::make_shared<Vector3>(-1, 1, 0), std::make_shared<Vector3>(1, 1, 0));
}
TubeGeometry::TubeGeometry(std::shared_ptr<Curve<Vector3>> path, double tubularSegments,
                           double radius, double radialSegments, bool closed)
    : parameters{path, tubularSegments, radius, radialSegments, closed} {
    type = "TubeGeometry";
    if (!path) throw JSTypeError("TubeGeometry path is null");
    auto frames = path->computeFrenetFrames(tubularSegments, closed);
    tangents = frames.tangents; normals = frames.normals; binormals = frames.binormals;
    std::vector<double> vertices, normalValues, uvs;
    std::vector<std::uint32_t> indices;
    Vector3 position, normal;
    const auto segment = [&](double i) {
        position.copy(path->getPointAt(i / tubularSegments, position));
        for (double j = 0; j <= radialSegments; ++j) {
            const auto v = j / radialSegments * std::numbers::pi * 2;
            const auto sine = std::sin(v), cosine = -std::cos(v);
            if (!(i >= 0 && i < double(frames.normals.size()) && std::trunc(i) == i))
                throw JSTypeError("TubeGeometry segment Frenet normal is undefined");
            if (radialSegments == INFINITY)
                throw JSRangeError("TubeGeometry infinite radial subdivision does not terminate");
            const auto& n = frames.normals[static_cast<std::size_t>(i)];
            const auto& b = frames.binormals[static_cast<std::size_t>(i)];
            normal.set(cosine * n.x + sine * b.x, cosine * n.y + sine * b.y, cosine * n.z + sine * b.z).normalize();
            normalValues.insert(normalValues.end(), {normal.x, normal.y, normal.z});
            vertices.insert(vertices.end(), {position.x + radius * normal.x,
                position.y + radius * normal.y, position.z + radius * normal.z});
        }
    };
    for (double i = 0; i < tubularSegments; ++i) segment(i);
    segment(closed ? 0 : tubularSegments);
    for (double i = 0; i <= tubularSegments; ++i)
        for (double j = 0; j <= radialSegments; ++j) uvs.insert(uvs.end(), {i / tubularSegments, j / radialSegments});
    for (double j = 1; j <= tubularSegments; ++j) {
        for (double i = 1; i <= radialSegments; ++i) {
            const auto a = (radialSegments + 1) * (j - 1) + (i - 1), b = (radialSegments + 1) * j + (i - 1),
                       c = (radialSegments + 1) * j + i, d = (radialSegments + 1) * (j - 1) + i;
            for (const auto index : {a, b, d, b, c, d}) indices.push_back(std::uint32_t(index));
        }
    }
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(normalValues, 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvs, 2));
}
BufferGeometry& TubeGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source); parameters = dynamic_cast<const TubeGeometry&>(source).parameters; return *this;
}
std::shared_ptr<BufferGeometry> TubeGeometry::clone() const {
    auto result = std::make_shared<TubeGeometry>(); result->copy(*this); return result;
}
JsonValue TubeGeometry::toJSON() const {
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    if (_transformed) { result = BufferGeometry::toJSON(); result["type"] = "BufferGeometry"; }
    else {
        result["path"] = nullptr;
        result["tubularSegments"] = parameters.tubularSegments; result["radius"] = parameters.radius;
        result["radialSegments"] = parameters.radialSegments; result["closed"] = parameters.closed;
    }
    result["path"] = parameters.path->toJSON(); return result;
}
std::shared_ptr<TubeGeometry> TubeGeometry::fromJSON(const JsonValue& data) {
    const auto& jsonPath = data.at("path");
    const auto type = jsonPath.at("type").get<std::string>();
    std::shared_ptr<Curve<Vector3>> path;
    if (type == "LineCurve3") path = std::make_shared<LineCurve3>();
    else if (type == "QuadraticBezierCurve3") path = std::make_shared<QuadraticBezierCurve3>();
    else if (type == "CubicBezierCurve3") path = std::make_shared<CubicBezierCurve3>();
    else if (type == "CatmullRomCurve3") path = std::make_shared<CatmullRomCurve3>();
    else throw JSTypeError("TubeGeometry.fromJSON requires a built-in spatial curve");
    path->fromJSON(jsonPath);
    const auto number = [&](const char* key, double fallback) { return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback; };
    return std::make_shared<TubeGeometry>(path, number("tubularSegments", 64), number("radius", 1),
        number("radialSegments", 8), data.contains("closed") ? data.at("closed").get<bool>() : false);
}
