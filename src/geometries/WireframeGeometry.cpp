#include "geometries/WireframeGeometry.h"
#include <unordered_set>

WireframeGeometry::WireframeGeometry(std::shared_ptr<BufferGeometry> geometry) : parameters{geometry} {
    type = "WireframeGeometry";
    if (!geometry) return;
    const auto position = geometry->getAttribute("position"); const auto index = geometry->index;
    if (!position) throw JSTypeError("WireframeGeometry position attribute is undefined");
    std::unordered_set<std::string> edges;
    std::vector<double> vertices;
    const auto hash = [](const Vector3& v) {
        return JSValue(v.x).toString() + "," + JSValue(v.y).toString() + "," + JSValue(v.z).toString();
    };
    const auto edge = [&](double first, double second) {
        Vector3 a, b; a.fromBufferAttribute(*position, first); b.fromBufferAttribute(*position, second);
        const auto hash1 = hash(a) + "-" + hash(b), hash2 = hash(b) + "-" + hash(a);
        if (edges.contains(hash1) || edges.contains(hash2)) return;
        edges.insert(hash1); edges.insert(hash2);
        vertices.insert(vertices.end(), {a.x, a.y, a.z, b.x, b.y, b.z});
    };
    if (index) {
        auto groups = geometry->groups;
        if (groups.empty()) {
            for (double i = 0; i < index->count; i += 3)
                for (int j = 0; j < 3; ++j) edge(index->getX(i + j), index->getX(i + (j + 1) % 3));
        } else {
            const auto count = groups.size();
            for (std::size_t groupIndex = 0; groupIndex < count; ++groupIndex) {
                const auto group = groups[groupIndex];
                const auto end = double(group.start) + double(group.count);
                for (double i = double(group.start); i < end; i += 3)
                    for (int j = 0; j < 3; ++j) edge(index->getX(i + j), index->getX(i + (j + 1) % 3));
            }
        }
    } else {
        for (double i = 0, count = position->getCount() / 3; i < count; ++i)
            for (int j = 0; j < 3; ++j) edge(3 * i + j, 3 * i + ((j + 1) % 3));
    }
    setAttribute("position", std::make_shared<Float32BufferAttribute>(vertices, 3));
}
BufferGeometry& WireframeGeometry::copy(const BufferGeometry& source) {
    BufferGeometry::copy(source); parameters = dynamic_cast<const WireframeGeometry&>(source).parameters; return *this;
}
std::shared_ptr<BufferGeometry> WireframeGeometry::clone() const {
    auto result = std::make_shared<WireframeGeometry>(); result->copy(*this); return result;
}
JsonValue WireframeGeometry::toJSON() const {
    if (_transformed) {
        auto result = BufferGeometry::toJSON(); result["type"] = "BufferGeometry"; return result;
    }
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "BufferGeometry"},
                                        {"generator", "BufferGeometry.toJSON"}}},
        {"uuid", uuid}, {"type", type}, {"name", name}};
    if (userData.size()) result["userData"] = userData.sharedCopy();
    result["geometry"] = parameters.geometry ? parameters.geometry->toJSON() : JsonValue(nullptr); return result;
}
