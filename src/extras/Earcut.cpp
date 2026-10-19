#include "extras/Earcut.h"

#include "extras/lib/earcut.h"

std::vector<std::size_t> Earcut::triangulate(const std::vector<double>& data,
                                             const std::vector<std::size_t>& holeIndices, std::size_t dim) {
    return ::earcut(data, holeIndices, dim);
}

JSValue Earcut::triangulate(JSValue data, JSValue holeIndices, JSValue dim) {
    std::vector<double> coordinates;
    const auto length = data.get("length").toNumber();
    for (double i = 0; i < length; i += 1) coordinates.push_back(data.get(i).toNumber());
    std::vector<std::size_t> holes;
    if (!holeIndices.isUndefined()) {
        const auto holeLength = holeIndices.get("length").toNumber();
        for (double i = 0; i < holeLength; i += 1) holes.push_back(static_cast<std::size_t>(holeIndices.get(i).toNumber()));
    }
    // Upstream defaults the dimension to two, which undefined means here.
    const auto dimensions = dim.isUndefined() ? std::size_t{2} : static_cast<std::size_t>(dim.toNumber());
    const auto faces = triangulate(coordinates, holes, dimensions);
    auto result = JSValue::array();
    for (std::size_t i = 0; i < faces.size(); i += 1) result.set(static_cast<double>(i), JSValue(static_cast<double>(faces[i])));
    return result;
}