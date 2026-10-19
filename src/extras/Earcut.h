#pragma once
#include "core/JSValue.h"
#include <cstddef>
#include <vector>

// Port of src/extras/Earcut.js at the pinned revision.
//
// Upstream is a ten line static wrapper around the bundled Mapbox earcut, and that algorithm was already ported
// here, so this is the same wrapper: triangulate delegates to it with the caller hole indices and dimension.
// The JSValue form converts an array of coordinates and an array of hole offsets, which is the shape the class
// is used with from JavaScript.
class Earcut {
  public:
    static std::vector<std::size_t> triangulate(const std::vector<double>& data,
                                                const std::vector<std::size_t>& holeIndices = {},
                                                std::size_t dim = 2);
    // The same call over JS values: data and holeIndices are arrays, dim defaults to two.
    static JSValue triangulate(JSValue data = {}, JSValue holeIndices = {}, JSValue dim = {});
};