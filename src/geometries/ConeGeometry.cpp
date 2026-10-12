#include "geometries/ConeGeometry.h"
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
ConeGeometry::ConeGeometry(double radius, double height, double radialSegments, double heightSegments, bool openEnded, double thetaStart, double thetaLength)
    : CylinderGeometry(0, radius, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength), parameters{radius, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength} {
    type = "ConeGeometry";

}
ConeGeometry& ConeGeometry::copy(const BufferGeometry& source) {
    CylinderGeometry::copy(source);
    if (const auto* geometry = dynamic_cast<const ConeGeometry*>(&source))
        parameters = geometry->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> ConeGeometry::clone() const {
    auto geometry = std::make_shared<ConeGeometry>();
    geometry->copy(*this);
    return geometry;
}
JsonValue ConeGeometry::toJSON() const {
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
    result["radialSegments"] = parameters.radialSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["openEnded"] = parameters.openEnded;
    result["thetaStart"] = parameters.thetaStart;
    result["thetaLength"] = parameters.thetaLength;
    return result;
}
std::shared_ptr<ConeGeometry> ConeGeometry::fromJSON(const JsonValue& data) {
    return std::make_shared<ConeGeometry>(jsonNumber(data, "radius", 1), jsonNumber(data, "height", 1), jsonNumber(data, "radialSegments", 32), jsonNumber(data, "heightSegments", 1), jsonBoolean(data, "openEnded", false), jsonNumber(data, "thetaStart", 0), jsonNumber(data, "thetaLength", std::numbers::pi * 2));
}
