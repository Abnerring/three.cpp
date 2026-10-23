#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Port of the key building and small lookups in examples/jsm/loaders/GLTFLoader.js at the
// pinned revision. A glTF file describes its primitives as plain JSON, and the loader caches
// geometry under a key it builds from the primitive, so the key has to be the same for two
// primitives that describe the same geometry. The parser itself needs a scene graph and a
// texture pipeline, so this part covers the key and the two small lookups.
class GLTFLoader {
  public:
    // The twelve bytes a binary glTF file begins with, the four character magic and the two
    // chunk kinds that follow the header.
    static constexpr const char* kBinaryMagic = "glTF";
    static constexpr int kBinaryHeaderLength = 12;
    static constexpr std::uint32_t kChunkJson = 0x4E4F534A;
    static constexpr std::uint32_t kChunkBin = 0x004E4942;

    // One attribute of a primitive: its name and the accessor it points at.
    using Attribute = std::pair<std::string, int>;

    // The key for a set of attributes. The names are sorted first so that two primitives that
    // list the same attributes in a different order give the same key, and each becomes
    // name, colon, accessor, semicolon.
    static std::string createAttributesKey(std::vector<Attribute> attributes);

    // The scale a normalised integer accessor needs: one over the largest value its type can
    // hold. A type that is not one of the four throws std::runtime_error.
    static double normalizedComponentScale(const std::string& componentType);

    // The mime type an image reference implies. A data uri names its own type, otherwise the
    // extension decides, and anything else is a png.
    static std::string imageMimeType(const std::string& uri);

    // The twelve byte header of a binary glTF file: four characters of magic, a version and the
    // total length, the last two little endian.
    struct BinaryHeader {
        std::string magic;
        std::uint32_t version = 0;
        std::uint32_t length = 0;
    };

    // Reads the header. Throws std::runtime_error when the magic is not the expected one or the
    // version is below two, both with the source message.
    static BinaryHeader readBinaryHeader(const std::vector<std::uint8_t>& buffer);

    // One chunk of a binary file. A chunk the loader does not know is reported with its type
    // so the caller can ignore it, which the format requires.
    struct BinaryChunk {
        std::uint32_t type = 0;
        std::size_t offset = 0;
        std::uint32_t length = 0;
    };

    // The chunks that follow the header, read in order. The loop stops when the declared file
    // length is reached rather than when the buffer ends.
    static std::vector<BinaryChunk> readBinaryChunks(const std::vector<std::uint8_t>& buffer,
                                                     const BinaryHeader& header);

    // The six component types the format allows, named the way the port names them.
    static std::string componentTypeName(std::uint32_t componentType);

    // How many bytes one component of a type takes. A type the format does not allow throws.
    static int componentTypeSize(std::uint32_t componentType);

    // The number of components an element of a shape holds, so a scalar is one and a four by
    // four matrix sixteen.
    static int typeSize(const std::string& type);

    // The attribute name a semantic maps to. A semantic not listed falls back to its own name
    // lower cased, which is what the parser does for the newer ones.
    static std::string attributeName(const std::string& semantic);

    // The filter a texture sampler names, as the port spells it. An unknown value throws.
    static std::string filterName(std::uint32_t filter);

    // The wrapping a sampler names. An unknown value throws.
    static std::string wrappingName(std::uint32_t wrapping);
};
