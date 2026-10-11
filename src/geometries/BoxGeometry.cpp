#include "geometries/BoxGeometry.h"
#include <array>
#include <cmath>
BoxGeometry::BoxGeometry(double width, double height, double depth, double widthSegments,
                         double heightSegments, double depthSegments)
    : parameters{width, height, depth, widthSegments, heightSegments, depthSegments} {
    type = "BoxGeometry";
    if (!std::isfinite(widthSegments) || !std::isfinite(heightSegments) ||
        !std::isfinite(depthSegments))
        throw std::invalid_argument("BoxGeometry segments must be finite");
    const int ws = static_cast<int>(std::floor(widthSegments)),
              hs = static_cast<int>(std::floor(heightSegments)),
              ds = static_cast<int>(std::floor(depthSegments));
    std::vector<float> vertices, normals, uvs;
    std::vector<std::uint32_t> indices;
    std::uint32_t vertexOffset = 0;
    std::size_t groupStart = 0;
    const auto buildPlane = [&](int u, int v, int w, int udir, int vdir, double width,
                                double height, double depth, int gx, int gy,
                                std::size_t materialIndex) {
        const double sw = width / gx, sh = height / gy;
        std::uint32_t vertexCount = 0;
        std::size_t groupCount = 0;
        for (int iy = 0; iy < gy + 1; ++iy)
            for (int ix = 0; ix < gx + 1; ++ix) {
                std::array<float, 3> p{}, n{};
                p[u] = static_cast<float>((ix * sw - width / 2) * udir);
                p[v] = static_cast<float>((iy * sh - height / 2) * vdir);
                p[w] = static_cast<float>(depth / 2);
                n[w] = depth > 0 ? 1.f : -1.f;
                vertices.insert(vertices.end(), p.begin(), p.end());
                normals.insert(normals.end(), n.begin(), n.end());
                uvs.insert(uvs.end(), {static_cast<float>(static_cast<double>(ix) / gx),
                                       static_cast<float>(1 - static_cast<double>(iy) / gy)});
                ++vertexCount;
            }
        for (int iy = 0; iy < gy; ++iy)
            for (int ix = 0; ix < gx; ++ix) {
                const auto a = vertexOffset + ix + (gx + 1) * iy,
                           b = vertexOffset + ix + (gx + 1) * (iy + 1),
                           c = vertexOffset + ix + 1 + (gx + 1) * (iy + 1),
                           d = vertexOffset + ix + 1 + (gx + 1) * iy;
                indices.insert(indices.end(), {a, b, d, b, c, d});
                groupCount += 6;
            }
        addGroup(groupStart, groupCount, materialIndex);
        groupStart += groupCount;
        vertexOffset += vertexCount;
    };
    buildPlane(2, 1, 0, -1, -1, depth, height, width, ds, hs, 0);
    buildPlane(2, 1, 0, 1, -1, depth, height, -width, ds, hs, 1);
    buildPlane(0, 2, 1, 1, 1, width, depth, height, ws, ds, 2);
    buildPlane(0, 2, 1, 1, -1, width, depth, -height, ws, ds, 3);
    buildPlane(0, 1, 2, 1, -1, width, height, depth, ws, hs, 4);
    buildPlane(0, 1, 2, -1, -1, width, height, -depth, ws, hs, 5);
    setIndex(indices);
    setAttribute("position", std::make_shared<Float32BufferAttribute>(std::move(vertices), 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(std::move(normals), 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(std::move(uvs), 2));
}
BufferGeometry& BoxGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    if (auto p = dynamic_cast<const BoxGeometry*>(&source))
        parameters = p->parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> BoxGeometry::clone() const {
    auto result = std::make_shared<BoxGeometry>();
    result->copy(*this);
    return result;
}
JsonValue BoxGeometry::toJSON() const {
    auto result = BufferGeometry::toJSON();
    if (_transformed) {
        result["type"] = "BufferGeometry";
        return result;
    }
    result.get<JsonValue::Object>().erase("data");
    result["width"] = parameters.width;
    result["height"] = parameters.height;
    result["depth"] = parameters.depth;
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["depthSegments"] = parameters.depthSegments;
    return result;
}
