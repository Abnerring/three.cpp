#include "geometries/PlaneGeometry.h"
#include <cmath>
PlaneGeometry::PlaneGeometry(double width, double height, double widthSegments,
                             double heightSegments)
    : parameters{width, height, widthSegments, heightSegments} {
    type = "PlaneGeometry";
    if (!std::isfinite(widthSegments) || !std::isfinite(heightSegments))
        throw std::invalid_argument("PlaneGeometry segments must be finite");
    const auto gx = static_cast<int>(std::floor(widthSegments)),
               gy = static_cast<int>(std::floor(heightSegments));
    const double sw = width / gx, sh = height / gy;
    std::vector<float> vertices, normals, uvs;
    std::vector<std::uint32_t> indices;
    for (int iy = 0; iy < gy + 1; ++iy)
        for (int ix = 0; ix < gx + 1; ++ix) {
            vertices.insert(vertices.end(), {static_cast<float>(ix * sw - width / 2),
                                             static_cast<float>(-(iy * sh - height / 2)), 0});
            normals.insert(normals.end(), {0, 0, 1});
            uvs.insert(uvs.end(), {static_cast<float>(static_cast<double>(ix) / gx),
                                   static_cast<float>(1 - static_cast<double>(iy) / gy)});
        }
    for (int iy = 0; iy < gy; ++iy)
        for (int ix = 0; ix < gx; ++ix) {
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
    auto result = BufferGeometry::toJSON();
    if (_transformed) {
        result["type"] = "BufferGeometry";
        return result;
    }
    result.get<JsonValue::Object>().erase("data");
    result["width"] = parameters.width;
    result["height"] = parameters.height;
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    return result;
}
