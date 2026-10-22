#pragma once
#include <array>
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

    // The twelve floats a MESH_MATRIX chunk holds, arranged into the sixteen elements of a
    // four by four matrix in the order the engine stores them. The chunk stores a row major
    // three by four; the arrangement below places each value, transposes, and the result is
    // what the loader applies to the geometry and then decomposes onto the mesh.
    static std::array<double, 16> meshMatrix(const std::array<double, 12>& values);

    // The triangles a face array chunk holds. Every face is three node numbers followed by a
    // fourth word of visibility flags that the loader reads and discards.
    struct Faces {
        std::vector<std::uint16_t> indices;
        std::uint16_t faceCount = 0;
    };

    // Reads the faces, leaving the cursor just past them and ready for the sub chunks that
    // follow.
    static Faces readFaces(const std::vector<std::uint8_t>& buffer, Chunk& chunk);

    // Where one material group's triangles sit. Each group covers as many indices as it names
    // times three, laid end to end in the order the groups appear.
    struct GroupRange {
        std::size_t start = 0;
        std::size_t count = 0;
        int materialIndex = 0;
    };

    // Turns the face counts of successive groups into the ranges the geometry wants.
    static std::vector<GroupRange> groupRanges(const std::vector<std::size_t>& facesPerGroup);

    // The value a shininess chunk gives. The percentage reader already divides by a hundred,
    // so it is multiplied back to reach the range the material expects.
    static double shininessFromPercentage(double percentage);

    // The opacity a transparency chunk gives: one minus the transparency, since the chunk
    // counts how much light passes through rather than how much is kept.
    static double opacityFromTransparency(double transparency);

    // A material group: the material's name and the face numbers it covers.
    struct MaterialGroup {
        std::string name;
        std::vector<std::uint16_t> index;
    };

    // Reads a group, leaving the cursor past it.
    static MaterialGroup readMaterialGroup(const std::vector<std::uint8_t>& buffer, Chunk& chunk);

    // The four values a map chunk can carry. Each defaults to what a texture starts with, since
    // a map that names none of them leaves the texture untouched.
    struct MapTransform {
        double offsetX = 0;
        double offsetY = 0;
        double repeatX = 1;
        double repeatY = 1;
        // The file name the map points at, empty when the chunk named no file.
        std::string name;
    };

    // Reads the values the map sub chunks carry. The chunk ids are given in order so the
    // caller can feed them one at a time as it walks the sub chunks.
    static void applyMapChunk(const std::vector<std::uint8_t>& buffer, Chunk& sub,
                              MapTransform& transform);
};
