#include "geometries/EdgesGeometry.h"
#include "math/MathUtils.h"
#include "math/Triangle.h"
#include <unordered_map>
#include <numbers>

EdgesGeometry::EdgesGeometry(std::shared_ptr<BufferGeometry> geometry, double thresholdAngle)
    : parameters{geometry, thresholdAngle} {
    type = "EdgesGeometry";
    if (!geometry) return;
    const auto index = geometry->getIndex(); const auto position = geometry->getAttribute("position");
    if (!position) throw JSTypeError("EdgesGeometry position attribute is undefined");
    const auto count = index ? index->count : position->getCount();
    const auto threshold = std::cos((std::numbers::pi / 180) * thresholdAngle);
    struct Edge { double index0, index1; Vector3 normal; };
    std::unordered_map<std::string, std::size_t> keys;
    std::vector<std::optional<Edge>> edges;
    std::vector<double> vertices;
    const auto hash = [](const Vector3& v) {
        return JSValue(MathUtils::round(v.x * 10000)).toString() + "," +
            JSValue(MathUtils::round(v.y * 10000)).toString() + "," + JSValue(MathUtils::round(v.z * 10000)).toString();
    };
    const auto push = [&](const Vector3& v) { vertices.insert(vertices.end(), {v.x, v.y, v.z}); };
    for (double i = 0; i < count; i += 3) {
        std::array<double, 3> indices{index ? index->getX(i) : i,
            index ? index->getX(i + 1) : i + 1, index ? index->getX(i + 2) : i + 2};
        std::array<Vector3, 3> points;
        for (int j = 0; j < 3; ++j) points[j].fromBufferAttribute(*position, indices[j]);
        Vector3 normal; Triangle::getNormal(points[0], points[1], points[2], normal);
        const std::array<std::string, 3> hashes{hash(points[0]), hash(points[1]), hash(points[2])};
        if (hashes[0] == hashes[1] || hashes[1] == hashes[2] || hashes[2] == hashes[0]) continue;
        for (std::size_t j = 0; j < 3; ++j) {
            const auto next = (j + 1) % 3;
            const auto forward = hashes[j] + "_" + hashes[next], reverse = hashes[next] + "_" + hashes[j];
            const auto found = keys.find(reverse);
            if (found != keys.end() && edges[found->second]) {
                if (normal.dot(edges[found->second]->normal) <= threshold) { push(points[j]); push(points[next]); }
                edges[found->second].reset();
            } else if (!keys.contains(forward)) {
                keys.emplace(forward, edges.size()); edges.push_back(Edge{indices[j], indices[next], normal});
            }
        }
    }
    for (const auto& edge : edges) if (edge) {
        Vector3 a, b; a.fromBufferAttribute(*position, edge->index0); b.fromBufferAttribute(*position, edge->index1);
        push(a); push(b);
    }
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
}
BufferGeometry& EdgesGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source); parameters = dynamic_cast<const EdgesGeometry&>(source).parameters; return *this;
}
std::shared_ptr<BufferGeometry> EdgesGeometry::clone() const {
    auto result = std::make_shared<EdgesGeometry>(); result->copy(*this); return result;
}
JsonValue EdgesGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON(); result["type"] = "BufferGeometry"; return result;
    }
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    // JsonValue represents serialized data, not a live BufferGeometry object.
    result["geometry"] = parameters.geometry ? parameters.geometry->toJSON() : JsonValue(nullptr);
    result["thresholdAngle"] = parameters.thresholdAngle; return result;
}
