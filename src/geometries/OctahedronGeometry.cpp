#include "geometries/OctahedronGeometry.h"
namespace {
PolyhedronGeometry::Numbers vertices() {  return {
			1, 0, 0, 	- 1, 0, 0,	0, 1, 0,
			0, - 1, 0, 	0, 0, 1,	0, 0, - 1
		}; }
PolyhedronGeometry::Numbers indices() { return {
			0, 2, 4,	0, 4, 3,	0, 3, 5,
			0, 5, 2,	1, 2, 5,	1, 5, 3,
			1, 3, 4,	1, 4, 2
		}; }
}
OctahedronGeometry::OctahedronGeometry(double radius, double detail)
    : PolyhedronGeometry(vertices(), indices(), radius, detail) {
    type = "OctahedronGeometry";
    parameters.vertices.reset(); parameters.indices.reset();
}
std::shared_ptr<BufferGeometry> OctahedronGeometry::clone() const {
    auto result = std::make_shared<OctahedronGeometry>(); result->copy(*this); return result;
}
std::shared_ptr<OctahedronGeometry> OctahedronGeometry::fromJSON(const JsonValue& data) {
    const auto number = [&](const char* key, double fallback) {
        return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback;
    };
    return std::make_shared<OctahedronGeometry>(number("radius", 1), number("detail", 0));
}
