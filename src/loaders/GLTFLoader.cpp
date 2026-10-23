#include "loaders/GLTFLoader.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
// The name of a component type as the file writes it, lower cased for comparison.
std::string lower(const std::string& text) {
    std::string out = text;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
// Whether a uri ends in the given extension, with an optional query string after it.
bool endsWithExtension(const std::string& uri, const std::string& extension) {
    const std::string lowered = lower(uri);
    std::size_t end = lowered.size();
    // A query string may follow the extension.
    const std::size_t query = lowered.find('?');
    if (query != std::string::npos) end = query;
    if (end < extension.size()) return false;
    return lowered.compare(end - extension.size(), extension.size(), extension) == 0;
}
}  // namespace

std::string GLTFLoader::createAttributesKey(std::vector<Attribute> attributes) {
    // Sorting the names is what lets two primitives that list the same attributes in a
    // different order share a key.
    std::sort(attributes.begin(), attributes.end(),
              [](const Attribute& a, const Attribute& b) { return a.first < b.first; });
    std::string key;
    for (const auto& attribute : attributes) {
        key += attribute.first;
        key += ':';
        key += std::to_string(attribute.second);
        key += ';';
    }
    return key;
}

double GLTFLoader::normalizedComponentScale(const std::string& componentType) {
    // The scale is one over the largest value the type can hold.
    const std::string type = lower(componentType);
    if (type == "int8") return 1.0 / 127.0;
    if (type == "uint8") return 1.0 / 255.0;
    if (type == "int16") return 1.0 / 32767.0;
    if (type == "uint16") return 1.0 / 65535.0;
    throw std::runtime_error(
        "THREE.GLTFLoader: Unsupported normalized accessor component type.");
}

std::string GLTFLoader::imageMimeType(const std::string& uri) {
    // A data uri names its own type.
    if (uri.rfind("data:image/jpeg", 0) == 0) return "image/jpeg";
    if (uri.rfind("data:image/webp", 0) == 0) return "image/webp";
    if (uri.rfind("data:image/ktx2", 0) == 0) return "image/ktx2";
    // Otherwise the extension decides, and a query string does not hide it.
    if (endsWithExtension(uri, ".jpg") || endsWithExtension(uri, ".jpeg")) return "image/jpeg";
    if (endsWithExtension(uri, ".webp")) return "image/webp";
    if (endsWithExtension(uri, ".ktx2")) return "image/ktx2";
    // Anything else is assumed to be a png.
    return "image/png";
}

namespace {
std::uint32_t readLE32(const std::vector<std::uint8_t>& buffer, std::size_t at) {
    return static_cast<std::uint32_t>(buffer[at]) |
           (static_cast<std::uint32_t>(buffer[at + 1]) << 8) |
           (static_cast<std::uint32_t>(buffer[at + 2]) << 16) |
           (static_cast<std::uint32_t>(buffer[at + 3]) << 24);
}
}  // namespace

GLTFLoader::BinaryHeader GLTFLoader::readBinaryHeader(const std::vector<std::uint8_t>& buffer) {
    if (buffer.size() < static_cast<std::size_t>(kBinaryHeaderLength))
        throw std::runtime_error("THREE.GLTFLoader: Unsupported glTF-Binary header.");

    BinaryHeader header;
    // The magic is four characters rather than a number.
    header.magic.assign(reinterpret_cast<const char*>(buffer.data()), 4);
    header.version = readLE32(buffer, 4);
    header.length = readLE32(buffer, 8);

    // A file that does not begin with the magic is not a binary glTF at all.
    if (header.magic != kBinaryMagic)
        throw std::runtime_error("THREE.GLTFLoader: Unsupported glTF-Binary header.");
    // The first version of the format laid its chunks out differently.
    if (header.version < 2)
        throw std::runtime_error("THREE.GLTFLoader: Legacy binary file detected.");
    return header;
}

std::vector<GLTFLoader::BinaryChunk> GLTFLoader::readBinaryChunks(
    const std::vector<std::uint8_t>& buffer, const BinaryHeader& header) {
    std::vector<BinaryChunk> chunks;
    // The chunks fill whatever the header length leaves.
    if (header.length < static_cast<std::uint32_t>(kBinaryHeaderLength)) return chunks;
    std::size_t contentsLength =
        static_cast<std::size_t>(header.length) - static_cast<std::size_t>(kBinaryHeaderLength);
    std::size_t index = 0;
    const std::size_t base = static_cast<std::size_t>(kBinaryHeaderLength);

    // Each chunk is a length and a type followed by that many bytes.
    while (index + 8 <= contentsLength && base + index + 8 <= buffer.size()) {
        const std::uint32_t chunkLength = readLE32(buffer, base + index);
        index += 4;
        const std::uint32_t chunkType = readLE32(buffer, base + index);
        index += 4;

        BinaryChunk chunk;
        chunk.type = chunkType;
        chunk.offset = base + index;
        chunk.length = chunkLength;
        chunks.push_back(chunk);

        // A chunk the caller does not know is skipped rather than refused.
        index += chunkLength;
    }
    return chunks;
}