#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Port of the rotation decoding in examples/jsm/loaders/VOXLoader.js at the pinned revision.
// A VOX transform stores a rotation as a single byte: two bits pick the column of the first
// two rows, three bits give their signs, and the third row takes the remaining column. The
// matrix is then turned from the VOX frame, where Z is up, into the port's Y up frame.
class VOXLoader {
  public:
    // The sixteen components of a column-major rotation matrix, ready for a Matrix4.
    using Matrix = std::array<double, 16>;

    // Decodes one rotation byte. Throws std::runtime_error when the two column indices are
    // equal, which leaves no third column to assign.
    static Matrix rotationMatrix(std::uint8_t byte);

    // One chunk: a four character identifier, the bytes that belong to it and the chunks
    // nested inside it. The format keeps the two lengths separate, so a reader must not
    // assume the children follow the content without a gap being possible.
    struct Chunk {
        std::string id;
        std::size_t contentOffset = 0;
        std::size_t contentSize = 0;
        std::vector<Chunk> children;
    };

    // A parsed file: the format version and the top level chunks, which normally means a
    // single MAIN.
    struct File {
        int version = 0;
        std::vector<Chunk> chunks;
    };

    // Splits the buffer into chunks. Throws std::runtime_error when the signature is missing
    // or a chunk claims more bytes than the buffer holds.
    static File parseChunks(const std::vector<std::uint8_t>& buffer);
};
