#include "geometries/TetrahedronGeometry.h"
namespace {
PolyhedronGeometry::Numbers vertices() {  return {
			1, 1, 1, 	- 1, - 1, 1, 	- 1, 1, - 1, 	1, - 1, - 1
		}; }
PolyhedronGeometry::Numbers indices() { return {
			2, 1, 0, 	0, 3, 2,	1, 3, 0,	2, 3, 1
		}; }
}
TetrahedronGeometry::TetrahedronGeometry(double radius, double detail)
    : PolyhedronGeometry(vertices(), indices(), radius, detail) {
    type = "TetrahedronGeometry";
    parameters.vertices.reset(); parameters.indices.reset();
}
std::shared_ptr<BufferGeometry> TetrahedronGeometry::clone() const {
    auto result = std::make_shared<TetrahedronGeometry>(); result->copy(*this); return result;
}
std::shared_ptr<TetrahedronGeometry> TetrahedronGeometry::fromJSON(const JsonValue& data) {
    const auto number = [&](const char* key, double fallback) {
        return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback;
    };
    return std::make_shared<TetrahedronGeometry>(number("radius", 1), number("detail", 0));
}
