#pragma once
#include "textures/Data3DTexture.h"

#include <memory>
#include <string>

// Port of examples/jsm/loaders/LUT3dlLoader.js at the pinned revision. As with the cube LUT
// loader only the parsing half is ported, because fetching a file is a host concern.
class LUT3dlLoader {
  public:
    struct Result {
        double size = 0;
        std::shared_ptr<Data3DTexture> texture3D;
    };

    LUT3dlLoader& setType(double type) {
        _type = type;
        return *this;
    }
    double type() const { return _type; }

    // Throws std::runtime_error with the upstream message when the grid line is missing or
    // not evenly spaced.
    Result parse(const std::string& input) const;

  private:
    double _type = UnsignedByteType;
};
