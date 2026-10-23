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

std::string GLTFLoader::componentTypeName(std::uint32_t componentType) {
    // The six types the format allows, by the number the file writes.
    switch (componentType) {
        case 5120: return "int8";
        case 5121: return "uint8";
        case 5122: return "int16";
        case 5123: return "uint16";
        case 5125: return "uint32";
        case 5126: return "float32";
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unsupported component type.");
    }
}

int GLTFLoader::componentTypeSize(std::uint32_t componentType) {
    // The width in bytes of one component.
    switch (componentType) {
        case 5120: return 1;
        case 5121: return 1;
        case 5122: return 2;
        case 5123: return 2;
        case 5125: return 4;
        case 5126: return 4;
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unsupported component type.");
    }
}

int GLTFLoader::typeSize(const std::string& type) {
    // A matrix holds as many components as its rows times its columns.
    if (type == "SCALAR") return 1;
    if (type == "VEC2") return 2;
    if (type == "VEC3") return 3;
    if (type == "VEC4") return 4;
    if (type == "MAT2") return 4;
    if (type == "MAT3") return 9;
    if (type == "MAT4") return 16;
    throw std::runtime_error("THREE.GLTFLoader: Unsupported accessor type.");
}

std::string GLTFLoader::attributeName(const std::string& semantic) {
    // The ten the parser names explicitly.
    if (semantic == "POSITION") return "position";
    if (semantic == "NORMAL") return "normal";
    if (semantic == "TANGENT") return "tangent";
    if (semantic == "TEXCOORD_0") return "uv";
    if (semantic == "TEXCOORD_1") return "uv1";
    if (semantic == "TEXCOORD_2") return "uv2";
    if (semantic == "TEXCOORD_3") return "uv3";
    if (semantic == "COLOR_0") return "color";
    if (semantic == "WEIGHTS_0") return "skinWeight";
    if (semantic == "JOINTS_0") return "skinIndex";
    // Anything else is its own name lower cased.
    std::string name = semantic;
    for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return name;
}

std::string GLTFLoader::filterName(std::uint32_t filter) {
    switch (filter) {
        case 9728: return "NearestFilter";
        case 9729: return "LinearFilter";
        case 9984: return "NearestMipmapNearestFilter";
        case 9985: return "LinearMipmapNearestFilter";
        case 9986: return "NearestMipmapLinearFilter";
        case 9987: return "LinearMipmapLinearFilter";
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unknown filter.");
    }
}

std::string GLTFLoader::wrappingName(std::uint32_t wrapping) {
    switch (wrapping) {
        case 33071: return "ClampToEdgeWrapping";
        case 33648: return "MirroredRepeatWrapping";
        case 10497: return "RepeatWrapping";
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unknown wrapping.");
    }
}

std::vector<std::uint8_t> GLTFLoader::sliceBufferView(const std::vector<std::uint8_t>& buffer,
                                                      std::size_t byteOffset,
                                                      std::size_t byteLength) {
    // A view that reaches past the buffer is cut at its end rather than refused, which is what
    // taking a slice does.
    if (byteOffset >= buffer.size() || byteLength == 0) return {};
    const std::size_t end = std::min(buffer.size(), byteOffset + byteLength);
    return std::vector<std::uint8_t>(buffer.begin() + static_cast<std::ptrdiff_t>(byteOffset),
                                     buffer.begin() + static_cast<std::ptrdiff_t>(end));
}

std::size_t GLTFLoader::accessorComponentCount(std::size_t count, const std::string& type) {
    // Each element holds as many components as its shape says.
    return count * static_cast<std::size_t>(typeSize(type));
}

bool GLTFLoader::isInterleaved(std::size_t byteStride, std::size_t itemBytes) {
    // No stride, or one that matches the element size, is the plain layout.
    return byteStride != 0 && byteStride != itemBytes;
}

std::size_t GLTFLoader::interleavedSlice(std::size_t byteOffset, std::size_t byteStride) {
    // Which run of elements the offset begins in.
    if (byteStride == 0) return 0;
    return byteOffset / byteStride;
}

std::size_t GLTFLoader::interleavedAttributeOffset(std::size_t byteOffset, std::size_t byteStride,
                                                   std::size_t elementBytes) {
    // The attribute begins part way into each element, counted in components.
    if (byteStride == 0 || elementBytes == 0) return 0;
    return (byteOffset % byteStride) / elementBytes;
}

std::size_t GLTFLoader::interleavedStride(std::size_t byteStride, std::size_t elementBytes) {
    // The buffer counts its stride in components rather than bytes.
    if (elementBytes == 0) return 0;
    return byteStride / elementBytes;
}

void GLTFLoader::applySparse(const std::vector<std::uint32_t>& indices,
                             const std::vector<double>& values, std::size_t itemSize,
                             std::vector<double>& target) {
    if (itemSize == 0) return;
    // Each index names one element and the values run on in that order.
    for (std::size_t entry = 0; entry < indices.size(); ++entry) {
        const std::size_t base = static_cast<std::size_t>(indices[entry]) * itemSize;
        for (std::size_t component = 0; component < itemSize; ++component) {
            // An element past the end, or a value run that stopped short, leaves what was there.
            if (base + component >= target.size()) break;
            const std::size_t source = entry * itemSize + component;
            if (source >= values.size()) break;
            target[base + component] = values[source];
        }
    }
}

bool GLTFLoader::sparseNormalized(bool /*normalized*/) {
    // Whatever the accessor declared, a sparse override reports itself as not normalised.
    return false;
}