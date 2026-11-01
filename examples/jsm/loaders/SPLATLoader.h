#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of examples/jsm/loaders/SPLATLoader.js at the pinned revision. Each 32 byte row is a
// centre (three floats, little endian), a scale (three floats), an RGBA colour and a
// quaternion whose components are stored as w, x, y, z in biased bytes.
class SPLATLoader {
  public:
    struct Result {
        std::vector<float> centers;      // three per splat
        std::vector<float> covariances;  // six per splat
        std::vector<std::uint8_t> colors;  // four per splat
    };

    static constexpr std::size_t kRowSizeBytes = 32;

    // Throws std::runtime_error when the byte length is not a whole number of rows.
    Result parse(const std::vector<std::uint8_t>& buffer) const;
};
