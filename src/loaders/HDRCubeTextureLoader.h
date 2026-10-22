#pragma once
#include "loaders/HDRLoader.h"
#include "textures/CubeTexture.h"

#include <memory>

// Port of examples/jsm/loaders/HDRCubeTextureLoader.js at the pinned revision. What is ported
// is the texture configuration and the data type forwarding; fetching six faces and filling
// images[i] is host work, so a caller parses six buffers with HDRLoader and hands the results
// in through setFace.
class HDRCubeTextureLoader {
  public:
    HDRCubeTextureLoader() = default;

    // Upstream defaults to half floats, which is what the cube variant wants.
    double type() const { return _type; }

    // Forwards to the owned HDR loader so both agree on the target type.
    HDRCubeTextureLoader& setDataType(double value) {
        _type = value;
        _hdrLoader.setDataType(value);
        return *this;
    }

    const HDRLoader& hdrLoader() const { return _hdrLoader; }

    // Builds the cube texture upstream creates at the top of load: the type is recorded and,
    // for the two float types only, the colour space and filtering are pinned to linear with
    // no mipmaps. Any other type leaves the defaults alone, matching the switch with two
    // identical cases and no default.
    std::shared_ptr<CubeTexture> createTexture() const;

    // Fills one face from an already parsed HDR image. Upstream stores a DataTexture per face
    // that copies the cube texture's settings.
    bool setFace(CubeTexture& texture, int index, const HDRLoader::Result& result) const;

  private:
    HDRLoader _hdrLoader;
    double _type = HDRLoader::kHalfFloatType;
};
