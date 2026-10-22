#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the chunk reading in examples/jsm/loaders/TDSLoader.js at the pinned revision. A 3DS
// file is a tree of chunks: a sixteen bit identifier, a thirty two bit byte count and then the
// contents, with every number little endian. The header is therefore six bytes, and the count
// covers the header as well as the contents. The mesh sections need a scene graph, so this part
// covers the cursor that walks the tree.
class TDSLoader {
  public:
    // A cursor over a chunk. The position begins just after the six byte header and stops at
    // the end the count declares.
    struct Chunk {
        std::uint16_t id = 0;
        std::uint32_t size = 0;
        std::size_t offset = 0;
        std::size_t position = 0;
        std::size_t end = 0;
    };

    // Reads the chunk header at the given offset. Throws std::runtime_error when the buffer is
    // too short to hold six bytes.
    static Chunk readChunk(const std::vector<std::uint8_t>& buffer, std::size_t offset);

    // Whether the cursor has reached the end of its chunk.
    static bool atEnd(const Chunk& chunk);

    // The readers. Each advances the cursor by what it read; a read past the end of the buffer
    // returns zero without advancing, which is the upstream fallback for a bad chunk.
    static std::uint8_t readByte(const std::vector<std::uint8_t>& buffer, Chunk& chunk);
    static std::uint16_t readWord(const std::vector<std::uint8_t>& buffer, Chunk& chunk);
    static std::uint32_t readDWord(const std::vector<std::uint8_t>& buffer, Chunk& chunk);
    static std::int16_t readShort(const std::vector<std::uint8_t>& buffer, Chunk& chunk);
    static std::int32_t readInt(const std::vector<std::uint8_t>& buffer, Chunk& chunk);
    static float readFloat(const std::vector<std::uint8_t>& buffer, Chunk& chunk);
    // A null terminated string, which consumes the terminator.
    static std::string readString(const std::vector<std::uint8_t>& buffer, Chunk& chunk);

    // A colour read from a sub chunk. The byte form divides by the byte range; the float form
    // is taken as it stands. Both go through the colour helper with the default space, so no
    // conversion happens and the numbers are the ones in the file.
    struct Rgb {
        double r = 1;
        double g = 1;
        double b = 1;
        // False when the sub chunk named a form the reader does not know, in which case the
        // colour stays at the helper's default of white.
        bool known = false;
    };

    // Reads the colour held in the sub chunk the parent points at.
    static Rgb readColor(const std::vector<std::uint8_t>& buffer, Chunk& parent);

    // Reads a percentage, which is a short divided by a hundred or a float taken as it stands.
    // An unknown form reads as zero.
    static double readPercentage(const std::vector<std::uint8_t>& buffer, Chunk& parent);
};
