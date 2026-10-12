#pragma once
#include <cstddef>
#include <vector>

// Direct port of three.js's bundled Mapbox earcut 3.0.2.
std::vector<std::size_t> earcut(const std::vector<double>& data,
                               const std::vector<std::size_t>& holeIndices = {}, std::size_t dim = 2);
double deviation(const std::vector<double>& data, const std::vector<std::size_t>& holeIndices,
                 std::size_t dim, const std::vector<std::size_t>& triangles);
struct EarcutFlattenResult {
    std::vector<double> vertices;
    std::vector<std::size_t> holes;
    std::size_t dimensions;
};
EarcutFlattenResult flatten(const std::vector<std::vector<std::vector<double>>>& data);
