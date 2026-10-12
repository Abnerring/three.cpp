#include "geometries/PolyhedronGeometry.h"
#include "math/MathUtils.h"
#include <numbers>

namespace {
double read(const std::vector<double>& values, double i) {
    return i >= 0 && i < values.size() && std::trunc(i) == i ? values[std::size_t(i)] : NAN;
}
double azimuth(const Vector3& v) { return std::atan2(v.z, -v.x); }
double jsonNumber(const JsonValue& data, const char* key, double fallback) {
    return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback;
}
}
PolyhedronGeometry::PolyhedronGeometry(NumbersPtr vertices, NumbersPtr indices, double radius, double detail)
    : parameters{vertices, indices, radius, detail} {
    type = "PolyhedronGeometry";
    if (!vertices || !indices) throw JSTypeError("PolyhedronGeometry requires vertices and indices arrays");
    if (detail == INFINITY) throw JSRangeError("PolyhedronGeometry infinite subdivision does not terminate");
    std::vector<double> vertexBuffer, uvBuffer;
    const auto vertexAt = [&](double index) {
        const auto stride = index * 3;
        return Vector3(read(*vertices, stride), read(*vertices, stride + 1), read(*vertices, stride + 2));
    };
    const auto push = [&](const Vector3& v) { vertexBuffer.insert(vertexBuffer.end(), {v.x, v.y, v.z}); };
    for (std::size_t face = 0; face < indices->size(); face += 3) {
        const auto a = vertexAt(read(*indices, double(face))), b = vertexAt(read(*indices, double(face + 1))),
                   c = vertexAt(read(*indices, double(face + 2)));
        const auto cols = detail + 1;
        std::vector<std::vector<Vector3>> grid;
        for (double i = 0; i <= cols; ++i) {
            auto aj = a; aj.lerp(c, i / cols);
            auto bj = b; bj.lerp(c, i / cols);
            const auto rows = cols - i;
            grid.emplace_back();
            for (double j = 0; j <= rows; ++j) {
                auto value = aj;
                if (!(j == 0 && i == cols)) value.lerp(bj, j / rows);
                grid.back().push_back(value);
            }
        }
        const auto at = [&](std::size_t i, std::size_t j) -> const Vector3& {
            if (i >= grid.size() || j >= grid[i].size()) throw JSTypeError("PolyhedronGeometry subdivision vertex is undefined");
            return grid[i][j];
        };
        for (std::size_t i = 0; i < cols; ++i) {
            for (std::size_t j = 0; j < 2 * (cols - i) - 1; ++j) {
                const auto k = j / 2;
                if (j % 2 == 0) { push(at(i, k + 1)); push(at(i + 1, k)); push(at(i, k)); }
                else { push(at(i, k + 1)); push(at(i + 1, k + 1)); push(at(i + 1, k)); }
            }
        }
    }
    for (std::size_t i = 0; i < vertexBuffer.size(); i += 3) {
        Vector3 vertex(vertexBuffer[i], vertexBuffer[i + 1], vertexBuffer[i + 2]);
        vertex.normalize().multiplyScalar(radius);
        vertexBuffer[i] = vertex.x; vertexBuffer[i + 1] = vertex.y; vertexBuffer[i + 2] = vertex.z;
    }
    for (std::size_t i = 0; i < vertexBuffer.size(); i += 3) {
        Vector3 v(vertexBuffer[i], vertexBuffer[i + 1], vertexBuffer[i + 2]);
        const auto u = azimuth(v) / 2 / std::numbers::pi + .5;
        const auto inclination = std::atan2(-v.y, std::sqrt(v.x * v.x + v.z * v.z));
        uvBuffer.insert(uvBuffer.end(), {u, 1 - (inclination / std::numbers::pi + .5)});
    }
    for (std::size_t i = 0, j = 0; i < vertexBuffer.size(); i += 9, j += 6) {
        Vector3 a(vertexBuffer[i], vertexBuffer[i + 1], vertexBuffer[i + 2]);
        Vector3 b(vertexBuffer[i + 3], vertexBuffer[i + 4], vertexBuffer[i + 5]);
        Vector3 c(vertexBuffer[i + 6], vertexBuffer[i + 7], vertexBuffer[i + 8]);
        auto center = a; center.add(b).add(c).divideScalar(3);
        const auto azi = azimuth(center);
        const auto correct = [&](const Vector3& v, std::size_t offset) {
            const auto u = uvBuffer[offset];
            if (azi < 0 && u == 1) uvBuffer[offset] = u - 1;
            if (v.x == 0 && v.z == 0) uvBuffer[offset] = azi / 2 / std::numbers::pi + .5;
        };
        correct(a, j); correct(b, j + 2); correct(c, j + 4);
    }
    for (std::size_t i = 0; i < uvBuffer.size(); i += 6) {
        const auto x0 = uvBuffer[i], x1 = uvBuffer[i + 2], x2 = uvBuffer[i + 4];
        const auto maximum = MathUtils::max(MathUtils::max(x0, x1), x2);
        const auto minimum = MathUtils::min(MathUtils::min(x0, x1), x2);
        if (maximum > .9 && minimum < .1) {
            if (x0 < .2) uvBuffer[i] += 1;
            if (x1 < .2) uvBuffer[i + 2] += 1;
            if (x2 < .2) uvBuffer[i + 4] += 1;
        }
    }
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertexBuffer, 3));
    setAttribute("normal", std::make_shared<Float32BufferAttribute>(vertexBuffer, 3));
    setAttribute("uv", std::make_shared<Float32BufferAttribute>(uvBuffer, 2));
    if (detail == 0) computeVertexNormals(); else normalizeNormals();
}
BufferGeometry& PolyhedronGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source);
    parameters = dynamic_cast<const PolyhedronGeometry&>(source).parameters;
    return *this;
}
std::shared_ptr<BufferGeometry> PolyhedronGeometry::clone() const {
    auto result = std::make_shared<PolyhedronGeometry>(); result->copy(*this); return result;
}
JsonValue PolyhedronGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON(); result["type"] = "BufferGeometry"; return result;
    }
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    for (const auto& [name, array] : {std::pair{"vertices", parameters.vertices}, std::pair{"indices", parameters.indices}})
        if (array) { JsonValue::Array data; for (const auto value : *array) data.emplace_back(value); result[name] = std::move(data); }
    result["radius"] = parameters.radius; result["detail"] = parameters.detail;
    return result;
}
std::shared_ptr<PolyhedronGeometry> PolyhedronGeometry::fromJSON(const JsonValue& data) {
    auto vertices = std::make_shared<Numbers>(), indices = std::make_shared<Numbers>();
    if (data.contains("vertices")) for (const auto& value : data.at("vertices").get<JsonValue::Array>()) vertices->push_back(value.isNull() ? 0 : value.get<double>());
    if (data.contains("indices")) for (const auto& value : data.at("indices").get<JsonValue::Array>()) indices->push_back(value.isNull() ? 0 : value.get<double>());
    return std::make_shared<PolyhedronGeometry>(vertices, indices, jsonNumber(data, "radius", 1), jsonNumber(data, "detail", 0));
}
