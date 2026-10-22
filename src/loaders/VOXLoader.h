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

    // The grid a model occupies, from the SIZE chunk.
    struct Size {
        int x = 0;
        int y = 0;
        int z = 0;
    };

    // One occupied cell: its coordinates and its index into the palette.
    struct Voxel {
        int x = 0;
        int y = 0;
        int z = 0;
        int colorIndex = 0;
    };

    // Reads the three sizes. Throws when the chunk is short.
    static Size readSize(const std::vector<std::uint8_t>& buffer, const Chunk& chunk);

    // Reads the voxel list: a count followed by four bytes per voxel.
    static std::vector<Voxel> readXyzi(const std::vector<std::uint8_t>& buffer, const Chunk& chunk);

    // Reads the palette as red green blue alpha entries. A palette chunk in practice holds
    // two hundred and fifty six entries, but the count is taken from the chunk size.
    static std::vector<std::array<std::uint8_t, 4>> readRgba(const std::vector<std::uint8_t>& buffer,
                                                             const Chunk& chunk);
};
