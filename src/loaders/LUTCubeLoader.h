#pragma once
#include "math/Vector3.h"
#include "textures/Data3DTexture.h"

#include <memory>
#include <optional>
#include <string>

// Port of examples/jsm/loaders/LUTCubeLoader.js at the pinned revision. Only the parsing
// half is ported; fetching a file is a host concern, so parse() takes the text directly.
class LUTCubeLoader {
  public:
    // Matches upstream, which returns this object literal from parse().
    struct Result {
        std::optional<std::string> title;
        double size = 0;
        Vector3 domainMin{0, 0, 0};
        Vector3 domainMax{1, 1, 1};
        std::shared_ptr<Data3DTexture> texture3D;
    };

    // Upstream defaults to UnsignedByteType and lets callers switch to FloatType.
    LUTCubeLoader& setType(double type) {
        _type = type;
        return *this;
    }
    double type() const { return _type; }

    // Throws std::runtime_error with the upstream message when the size line is missing or
    // the domain is inverted.
    Result parse(const std::string& input) const;

  private:
    double _type = UnsignedByteType;
};
