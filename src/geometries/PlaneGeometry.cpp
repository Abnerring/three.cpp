#include "geometries/PlaneGeometry.h"
#include <cmath>
PlaneGeometry::PlaneGeometry(double width, double height, double widthSegments,
                             double heightSegments)
    : parameters{width, height, widthSegments, heightSegments} {
    type = "PlaneGeometry";
    const double gx = std::floor(widthSegments), gy = std::floor(heightSegments);
    if (gy == INFINITY || (gy + 1 > 0 && gx == INFINITY))
        throw JSRangeError("PlaneGeometry infinite subdivision does not terminate");
    const double sw = width / gx, sh = height / gy;
    std::vector<double> vertices, normals, uvs;
    std::vector<std::uint32_t> indices;
    for (double iy = 0; iy < gy + 1; ++iy)
        for (double ix = 0; ix < gx + 1; ++ix) {
            vertices.insert(vertices.end(), {ix * sw - width / 2, -(iy * sh - height / 2), 0});
            normals.insert(normals.end(), {0, 0, 1});
            uvs.insert(uvs.end(), {ix / gx, 1 - iy / gy});
        }
    for (double iy = 0; iy < gy; ++iy)
        for (double ix = 0; ix < gx; ++ix) {
            const auto a = static_cast<std::uint32_t>(ix + (gx + 1) * iy),
                       b = static_cast<std::uint32_t>(ix + (gx + 1) * (iy + 1)),
                       c = static_cast<std::uint32_t>(ix + 1 + (gx + 1) * (iy + 1)),
                       d = static_cast<std::uint32_t>(ix + 1 + (gx + 1) * iy);
            indices.insert(indices.end(), {a, b, d, b, c, d});
        }
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(std::move(vertices), 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(std::move(normals), 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(std::move(uvs), 2));
}
BufferGeometry& PlaneGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (auto p = dynamic_cast<const PlaneGeometry*>(&source))
        parameters = p->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> PlaneGeometry::clone() const {
    auto result = std::make_shared<PlaneGeometry>();
    result->copy(*this);
    return result;
}
JsonValue PlaneGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON();
        result["type"] = "BufferGeometry";
        return result;
    }
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    result["width"] = parameters.width;
    result["height"] = parameters.height;
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    return result;
}
std::shared_ptr<PlaneGeometry> PlaneGeometry::fromJSON(const JsonValue& json) {
    const auto number = [&](const char* key) { return json.contains(key) ? JSValue::fromJSON(json.at(key)).toNumber() : 1.; };
    return std::make_shared<PlaneGeometry>(number("width"), number("height"), number("widthSegments"), number("heightSegments"));
}
