#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the container detection in examples/jsm/loaders/PVRLoader.js at the pinned revision.
// A PVR file is a compressed texture container with two on disk layouts: the third version
// leads with its signature and the second puts it at the end, so a reader has to look in both
// places. The compressed payload needs a GPU format the port does not carry, so only the
// detection and the header fields are portable.
class PVRLoader {
  public:
    // The number of words the common header occupies.
    static constexpr int kHeaderWords = 13;

    // Which layout a buffer holds: three, two, or zero when neither signature is found.
    static int detectVersion(const std::vector<std::uint8_t>& buffer);

    // Reads one little endian word of the header.
    static std::uint32_t headerWord(const std::vector<std::uint8_t>& buffer, int index);
};
