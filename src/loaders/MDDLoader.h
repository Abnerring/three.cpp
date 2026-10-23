#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Port of examples/jsm/loaders/MDDLoader.js at the pinned revision. A file is a frame count and
// a point count, then one time per frame, then the positions of every point in every frame.
class MDDLoader {
  public:
    // The two numbers the file begins with.
    struct Header {
        std::uint32_t frames = 0;
        std::uint32_t points = 0;
    };

    static Header header(const std::vector<std::uint8_t>& buffer);

    // How many numbers the timing section holds, one per frame.
    static std::size_t timeCount(const Header& header);

    // How many a frame's positions hold: three per point, with nothing between them.
    static std::size_t positionCount(const Header& header);

    // How many bytes a whole file of this shape takes, which is the header, one float per frame
    // and three per point per frame.
    static std::size_t byteSize(const Header& header);

    // The times, in the order the file writes them.
    static std::vector<double> times(const std::vector<std::uint8_t>& buffer, const Header& header);

    // The influence matrix the clip is built with. It is square, one row and one column per
    // frame, and holds a single one in each row: row i has its one at column i, so each frame
    // turns exactly one morph target all the way on and the rest off. Reading it as a flat array
    // means the one in row i sits at frame i's index plus i.
    static std::vector<double> influences(const Header& header);

    // The name a frame's morph target is given.
    static std::string morphName(std::uint32_t frame);

    // The positions of one frame, read from the buffer. A frame past the end gives nothing.
    static std::vector<double> framePositions(const std::vector<std::uint8_t>& buffer,
                                              const Header& header, std::uint32_t frame);
};
