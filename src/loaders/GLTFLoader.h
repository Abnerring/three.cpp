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
};
