#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the container and path helpers in examples/jsm/loaders/LWOLoader.js at the pinned
// revision. A LightWave object file is a tree of chunks: a four character identifier, a big
// endian byte count and then the contents, with the whole file wrapped in a form chunk that
// names the version. The rest of the loader needs texture loading and a scene graph, so this
// part covers the chunk frame and the path rule.
class LWOLoader {
  public:
    // The four characters of the wrapper chunk and the two version tags it can carry.
    static constexpr const char* kFormId = "FORM";

    struct ChunkHeader {
        // The four characters, spaces trimmed.
        std::string id;
        // The byte count that follows the identifier.
        std::uint32_t size = 0;
        // Where the contents begin.
        std::size_t dataOffset = 0;
    };

    // Reads one chunk header. Throws std::runtime_error when the buffer is too short to hold
    // one.
    static ChunkHeader readChunkHeader(const std::vector<std::uint8_t>& buffer,
                                       std::size_t offset = 0);

    // The part of a url before the given directory, or a single dot and slash when the
    // directory does not appear at all.
    static std::string extractParentUrl(const std::string& url, const std::string& dir);
};
