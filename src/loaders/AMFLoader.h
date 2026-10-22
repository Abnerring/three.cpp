#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the container and unit handling in examples/jsm/loaders/AMFLoader.js at the pinned
// revision. AMF is an additive manufacturing format: either plain XML or a zip archive that
// holds one, with a unit on the root element that scales the millimetre coordinates. This
// slice covers the archive detection and the unit scale; reading the mesh is the layer above.
class AMFLoader {
  public:
    // Whether the buffer starts with the zip signature, which upstream takes to mean the
    // document is packed and needs a decompressor to reach.
    static bool isZip(const std::vector<std::uint8_t>& buffer);

    // The millimetres one of the given unit equals. An unknown or missing unit is a
    // millimetre, which is what upstream falls back to.
    static double unitScale(const std::string& unit);
};
