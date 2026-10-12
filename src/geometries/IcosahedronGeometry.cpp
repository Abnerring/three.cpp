#include "geometries/IcosahedronGeometry.h"
namespace {
PolyhedronGeometry::Numbers vertices() { const double t = (1 + std::sqrt(5.)) / 2; return {
			- 1, t, 0, 	1, t, 0, 	- 1, - t, 0, 	1, - t, 0,
			0, - 1, t, 	0, 1, t,	0, - 1, - t, 	0, 1, - t,
			t, 0, - 1, 	t, 0, 1, 	- t, 0, - 1, 	- t, 0, 1
		}; }
PolyhedronGeometry::Numbers indices() { return {
			0, 11, 5, 	0, 5, 1, 	0, 1, 7, 	0, 7, 10, 	0, 10, 11,
			1, 5, 9, 	5, 11, 4,	11, 10, 2,	10, 7, 6,	7, 1, 8,
			3, 9, 4, 	3, 4, 2,	3, 2, 6,	3, 6, 8,	3, 8, 9,
			4, 9, 5, 	2, 4, 11,	6, 2, 10,	8, 6, 7,	9, 8, 1
		}; }
}
IcosahedronGeometry::IcosahedronGeometry(double radius, double detail)
    : PolyhedronGeometry(vertices(), indices(), radius, detail) {
    type = "IcosahedronGeometry";
    parameters.vertices.reset(); parameters.indices.reset();
}
std::shared_ptr<BufferGeometry> IcosahedronGeometry::clone() const {
    auto result = std::make_shared<IcosahedronGeometry>(); result->copy(*this); return result;
}
std::shared_ptr<IcosahedronGeometry> IcosahedronGeometry::fromJSON(const JsonValue& data) {
    const auto number = [&](const char* key, double fallback) {
        return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback;
    };
    return std::make_shared<IcosahedronGeometry>(number("radius", 1), number("detail", 0));
}
