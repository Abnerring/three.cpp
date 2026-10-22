#pragma once
#include <array>
#include <cstdint>
#include <map>
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

    // Builds the occupancy grid the 3D texture uses: one byte per cell, set to 255 where a
    // voxel sits and left at zero elsewhere. The palette is not consulted, so the texture
    // records shape rather than colour. Cells outside the declared size are ignored.
    static std::vector<std::uint8_t> buildOccupancy(const std::vector<Voxel>& voxels, const Size& size);

    // Builds the volume the mesher walks: one byte per cell holding the palette index of the
    // voxel there, or zero when the cell is empty. Note this differs from the occupancy grid,
    // which writes a flat two hundred and fifty five and is for the texture path.
    static std::vector<std::uint8_t> buildColorVolume(const std::vector<Voxel>& voxels,
                                                      const Size& size);

    // Builds the face mask for one slice of one axis. For each cell of the slice plane the
    // mask records the palette index when only the cell behind is solid (a negative facing
    // face), minus the index when only the cell in front is solid (a positive facing face),
    // and zero when both sides agree. The plane is walked with v outer and u inner, where u
    // and v are the two axes after d.
    static std::vector<int> buildMask(const std::vector<std::uint8_t>& volume, const Size& size,
                                      int d, int slice);

    // A merged rectangle on a slice plane: where it starts in u and v, how far it extends,
    // and the mask value it came from, which carries the sign of the facing.
    struct Quad {
        int u = 0;
        int v = 0;
        int w = 1;
        int h = 1;
        int value = 0;
    };

    // Greedily merges the equal valued neighbours of a mask into rectangles. The width grows
    // along u while the value matches, then the height grows along v only while every cell of
    // the whole width matches, which is what makes the result a rectangle rather than a run.
    static std::vector<Quad> mergeMask(std::vector<int> mask, int dimsU, int dimsV);

    // A quad emitted as geometry: four corners in triangle order and six indices.
    struct QuadGeometry {
        double vertices[12] = {0};
        std::array<int, 6> indices{0, 0, 0, 0, 0, 0};
    };

    // Turns one rectangle into a quad. The rectangle sits at pos on axis d and spans du in u
    // and dv in v; a positive value keeps the corners in that order while a negative one
    // swaps two of them so the face points the other way.
    static QuadGeometry emitQuad(const Quad& quad, const int pos[3], const int du[3],
                                 const int dv[3], const Size& size);

    // Converts one VOX point to the port frame: x keeps, y takes the old z, z takes the
    // negated old y, and the whole model is centred by subtracting half of each size.
    static std::array<double, 3> toThree(const int point[3], const Size& size);

    // The geometry a mesh chunk produces.
    struct MeshGeometry {
        std::vector<double> positions;
        std::vector<int> indices;
        // Three working space components per corner.
        std::vector<double> colors;
    };

    // Runs the mesher over the three axes: every slice is masked, merged into rectangles and
    // each rectangle becomes a quad. A palette entry is chosen by the magnitude of the mask
    // value, since the sign only carried the facing.
    static MeshGeometry buildMesh(const std::vector<Voxel>& voxels, const Size& size,
                                  const std::vector<std::array<std::uint8_t, 4>>& palette);

    // A length prefixed string: four bytes of length then that many bytes of text.
    struct StringValue {
        std::string value;
        std::size_t size = 0;
    };
    static StringValue readString(const std::vector<std::uint8_t>& buffer, std::size_t offset);

    // A dictionary: four bytes of entry count then that many key and value strings, all
    // length prefixed. The transform chunks carry their attributes this way.
    static std::map<std::string, std::string> readDict(const std::vector<std::uint8_t>& buffer,
                                                       std::size_t offset,
                                                       std::size_t& bytesRead);

    // One transform node as the file writes it: a name, an optional whole voxel translation
    // and an optional rotation byte. The dictionary keys are the ones the format uses.
    struct Transform {
        int nodeId = -1;
        std::string name;
        double translation[3] = {0, 0, 0};
        bool hasTranslation = false;
        int rotation = 0;
        bool hasRotation = false;
    };

    // Reads the attributes of an nTRN dictionary. A value that is not a number is treated as
    // absent rather than as zero, so a malformed file leaves the node untransformed.
    static Transform parseTransform(const std::map<std::string, std::string>& dict);

    // Reads the child list of an nGRP or nSHP dictionary.
    static std::vector<int> parseChildren(const std::map<std::string, std::string>& dict);
};
