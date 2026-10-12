#include "geometries/BoxGeometry.h"
#include <array>
#include <cmath>
BoxGeometry::BoxGeometry(double width, double height, double depth, double widthSegments,
                         double heightSegments, double depthSegments)
    : parameters{width, height, depth, widthSegments, heightSegments, depthSegments} {
    type = "BoxGeometry";
    const double ws = std::floor(widthSegments), hs = std::floor(heightSegments), ds = std::floor(depthSegments);
    std::vector<double> vertices, normals, uvs;
    std::vector<std::uint32_t> indices;
    std::uint32_t vertexOffset = 0;
    std::size_t groupStart = 0;
    const auto buildPlane = [&](int u, int v, int w, int udir, int vdir, double width,
                                double height, double depth, double gx, double gy,
                                std::size_t materialIndex) {
        if (gy == INFINITY || (gy + 1 > 0 && gx == INFINITY))
            throw JSRangeError("BoxGeometry infinite subdivision does not terminate");
        const double sw = width / gx, sh = height / gy;
        std::uint32_t vertexCount = 0;
        std::size_t groupCount = 0;
        for (double iy = 0; iy < gy + 1; ++iy)
            for (double ix = 0; ix < gx + 1; ++ix) {
                std::array<double, 3> p{}, n{};
                p[u] = (ix * sw - width / 2) * udir;
                p[v] = (iy * sh - height / 2) * vdir;
                p[w] = depth / 2;
                n[w] = depth > 0 ? 1 : -1;
                vertices.insert(vertices.end(), p.begin(), p.end());
                normals.insert(normals.end(), n.begin(), n.end());
                uvs.insert(uvs.end(), {ix / gx, 1 - iy / gy});
                ++vertexCount;
            }
        for (double iy = 0; iy < gy; ++iy)
            for (double ix = 0; ix < gx; ++ix) {
                const auto a = vertexOffset + ix + (gx + 1) * iy,
                           b = vertexOffset + ix + (gx + 1) * (iy + 1),
                           c = vertexOffset + ix + 1 + (gx + 1) * (iy + 1),
                           d = vertexOffset + ix + 1 + (gx + 1) * iy;
                for (double index : {a, b, d, b, c, d}) indices.push_back(static_cast<std::uint32_t>(index));
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
    result["depth"] = parameters.depth;
    result["widthSegments"] = parameters.widthSegments;
    result["heightSegments"] = parameters.heightSegments;
    result["depthSegments"] = parameters.depthSegments;
    return result;
}
std::shared_ptr<BoxGeometry> BoxGeometry::fromJSON(const JsonValue& json) {
    const auto number = [&](const char* key) { return json.contains(key) ? JSValue::fromJSON(json.at(key)).toNumber() : 1.; };
    return std::make_shared<BoxGeometry>(number("width"), number("height"), number("depth"),
        number("widthSegments"), number("heightSegments"), number("depthSegments"));
}
