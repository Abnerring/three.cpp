#include "geometries/DodecahedronGeometry.h"
namespace {
PolyhedronGeometry::Numbers vertices() { const double t = (1 + std::sqrt(5.)) / 2, r = 1 / t; return {


			- 1, - 1, - 1,	- 1, - 1, 1,
			- 1, 1, - 1, - 1, 1, 1,
			1, - 1, - 1, 1, - 1, 1,
			1, 1, - 1, 1, 1, 1,


			0, - r, - t, 0, - r, t,
			0, r, - t, 0, r, t,


			- r, - t, 0, - r, t, 0,
			r, - t, 0, r, t, 0,


			- t, 0, - r, t, 0, - r,
			- t, 0, r, t, 0, r
		}; }
PolyhedronGeometry::Numbers indices() { return {
			3, 11, 7, 	3, 7, 15, 	3, 15, 13,
			7, 19, 17, 	7, 17, 6, 	7, 6, 15,
			17, 4, 8, 	17, 8, 10, 	17, 10, 6,
			8, 0, 16, 	8, 16, 2, 	8, 2, 10,
			0, 12, 1, 	0, 1, 18, 	0, 18, 16,
			6, 10, 2, 	6, 2, 13, 	6, 13, 15,
			2, 16, 18, 	2, 18, 3, 	2, 3, 13,
			18, 1, 9, 	18, 9, 11, 	18, 11, 3,
			4, 14, 12, 	4, 12, 0, 	4, 0, 8,
			11, 9, 5, 	11, 5, 19, 	11, 19, 7,
			19, 5, 14, 	19, 14, 4, 	19, 4, 17,
			1, 12, 14, 	1, 14, 5, 	1, 5, 9
		}; }
}
DodecahedronGeometry::DodecahedronGeometry(double radius, double detail)
    : PolyhedronGeometry(vertices(), indices(), radius, detail) {
    type = "DodecahedronGeometry";
    parameters.vertices.reset(); parameters.indices.reset();
}
std::shared_ptr<BufferGeometry> DodecahedronGeometry::clone() const {
    auto result = std::make_shared<DodecahedronGeometry>(); result->copy(*this); return result;
}
std::shared_ptr<DodecahedronGeometry> DodecahedronGeometry::fromJSON(const JsonValue& data) {
    const auto number = [&](const char* key, double fallback) {
        return data.contains(key) ? (data.at(key).isNull() ? 0 : data.at(key).get<double>()) : fallback;
    };
    return std::make_shared<DodecahedronGeometry>(number("radius", 1), number("detail", 0));
}
