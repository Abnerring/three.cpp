#include "loaders/PCDLoader.h"

#include "core/BufferGeometry.h"
#include "math/Color.h"
#include "math/ColorManagement.h"
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string lower(std::string text) {
    for (auto& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::vector<std::string> splitWhitespace(const std::string& text) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) parts.push_back(token);
    return parts;
}
}  // namespace

std::vector<std::uint8_t> PCDLoader::decompressLZF(const std::vector<std::uint8_t>& inData,
                                                    std::size_t outLength) {
    const std::size_t inLength = inData.size();
    std::vector<std::uint8_t> outData(outLength, 0);
    std::size_t inPtr = 0;
    std::size_t outPtr = 0;

    do {
        const std::uint8_t ctrl = inData[inPtr++];
        if (ctrl < (1u << 5)) {
            // A literal run: the control byte counts the bytes minus one.
            std::size_t count = static_cast<std::size_t>(ctrl) + 1;
            if (outPtr + count > outLength)
                throw std::runtime_error("THREE.PCDLoader: Output buffer is not large enough");
            if (inPtr + count > inLength)
                throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            while (count--) outData[outPtr++] = inData[inPtr++];
        } else {
            // A back reference: length in the top three bits, offset in the low five.
            std::size_t length = ctrl >> 5;
            std::ptrdiff_t ref = static_cast<std::ptrdiff_t>(outPtr) -
                                 static_cast<std::ptrdiff_t>((ctrl & 0x1f) << 8) - 1;
            if (inPtr >= inLength)
                throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            if (length == 7) {
                length += inData[inPtr++];
                if (inPtr >= inLength)
                    throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            }
            ref -= inData[inPtr++];
            if (outPtr + length + 2 > outLength)
                throw std::runtime_error("THREE.PCDLoader: Output buffer is not large enough");
            if (ref < 0) throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            if (static_cast<std::size_t>(ref) >= outPtr)
                throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            // The run copies length plus two bytes.
            std::size_t remaining = length + 2;
            while (remaining--) outData[outPtr++] = outData[static_cast<std::size_t>(ref++)];
        }
    } while (inPtr < inLength);
    return outData;
}

PCDLoader::Header PCDLoader::parseHeader(const std::vector<std::uint8_t>& buffer) {
    Header header;
    std::string line;
    std::size_t i = 0;
    bool end = false;

    while (i < buffer.size() && !end) {
        const char character = static_cast<char>(buffer[i++]);
        if (character == '\n' || character == '\r') {
            const std::string text = trim(line);
            line.clear();
            if (text.empty()) continue;
            // The data line ends the header and names the storage.
            if (lower(text).rfind("data", 0) == 0) {
                header.data = trim(text.substr(4));
                end = true;
                break;
            }
            const auto tokens = splitWhitespace(text);
            if (tokens.empty()) continue;
            const std::string key = lower(tokens[0]);
            if (key == "fields") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.fields.push_back(lower(tokens[k]));
            } else if (key == "size") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.size.push_back(std::atoi(tokens[k].c_str()));
            } else if (key == "type") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.type.push_back(lower(tokens[k]));
            } else if (key == "count") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.count.push_back(std::atoi(tokens[k].c_str()));
            } else if (key == "width") {
                header.width = tokens.size() > 1 ? std::atoi(tokens[1].c_str()) : 0;
            } else if (key == "height") {
                header.height = tokens.size() > 1 ? std::atoi(tokens[1].c_str()) : 0;
            } else if (key == "points") {
                header.points = tokens.size() > 1 ? std::atoi(tokens[1].c_str()) : 0;
            }
        } else {
            line.push_back(character);
        }
    }
    header.headerLength = i;
    if (header.data.empty()) throw std::runtime_error("THREE.PCDLoader: Missing data declaration");
    return header;
}

PCDLoader::FieldLayout PCDLoader::computeOffsets(const Header& header) {
    FieldLayout layout;
    int sizeSum = 0;
    int countSum = 0;
    const bool ascii = header.data == "ascii";

    for (std::size_t i = 0; i < header.fields.size(); ++i) {
        const std::string& field = header.fields[i];
        const int fieldCount = i < header.count.size() ? header.count[i] : 1;
        if (ascii) {
            // Ascii offsets count tokens, so a field starts after the previous field's values.
            layout.offset[field] = countSum;
            countSum += fieldCount;
        } else {
            // Binary offsets count bytes, so a field starts after the previous field's storage.
            layout.offset[field] = sizeSum;
            const int fieldSize = i < header.size.size() ? header.size[i] : 1;
            sizeSum += fieldSize * fieldCount;
        }
    }
    // The row size only describes the binary forms.
    layout.rowSize = sizeSum;
    return layout;
}

void PCDLoader::unpackRGB(unsigned int packed, int rgb[3]) {
    // Red sits in the highest byte, then green, then blue.
    rgb[0] = static_cast<int>((packed >> 16) & 0xffu);
    rgb[1] = static_cast<int>((packed >> 8) & 0xffu);
    rgb[2] = static_cast<int>(packed & 0xffu);
}

PCDLoader::Points PCDLoader::parseAsciiPoints(const std::string& text, const FieldLayout& layout) {
    Points points;
    const auto offsetOf = [&layout](const std::string& name) -> int {
        const auto found = layout.offset.find(name);
        return found == layout.offset.end() ? -1 : found->second;
    };
    const int xAt = offsetOf("x");
    const int yAt = offsetOf("y");
    const int zAt = offsetOf("z");
    const int rgbAt = offsetOf("rgb");
    const int nxAt = offsetOf("normal_x");
    const int nyAt = offsetOf("normal_y");
    const int nzAt = offsetOf("normal_z");
    const int intensityAt = offsetOf("intensity");
    const int labelAt = offsetOf("label");

    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        const std::string raw =
            end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        start = end == std::string::npos ? text.size() + 1 : end + 1;

        const std::string line = trim(raw);
        if (line.empty()) continue;
        const auto tokens = splitWhitespace(line);

        const auto tokenAt = [&tokens](int index) -> double {
            if (index < 0 || static_cast<std::size_t>(index) >= tokens.size()) return 0.0;
            return std::strtod(tokens[static_cast<std::size_t>(index)].c_str(), nullptr);
        };

        // A position needs all three components to be worth recording.
        if (xAt >= 0 && yAt >= 0 && zAt >= 0) {
            points.positions.push_back(tokenAt(xAt));
            points.positions.push_back(tokenAt(yAt));
            points.positions.push_back(tokenAt(zAt));
        }
        if (nxAt >= 0 && nyAt >= 0 && nzAt >= 0) {
            points.normals.push_back(tokenAt(nxAt));
            points.normals.push_back(tokenAt(nyAt));
            points.normals.push_back(tokenAt(nzAt));
        }
        if (rgbAt >= 0) {
            // The packed value arrives as a number and is unpacked into channel bytes, then
            // converted out of sRGB into the working space.
            int rgb[3] = {0, 0, 0};
            unpackRGB(static_cast<unsigned int>(tokenAt(rgbAt)), rgb);
            const std::vector<double> components{rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0};
            Color color;
            color.fromArray(components);
            ColorManagement::colorSpaceToWorking(color, SRGBColorSpace);
            points.colors.push_back(color.r);
            points.colors.push_back(color.g);
            points.colors.push_back(color.b);
        }
        if (intensityAt >= 0) points.intensity.push_back(tokenAt(intensityAt));
        if (labelAt >= 0) points.label.push_back(tokenAt(labelAt));
    }
    return points;
}

PCDLoader::Points PCDLoader::parseBinaryPoints(const std::vector<std::uint8_t>& data,
                                               std::size_t start, int pointCount,
                                               const Header& header, const FieldLayout& layout,
                                               bool littleEndian) {
    Points points;
    // Index the metadata by field name so a lookup per field per point stays simple.
    std::map<std::string, std::pair<int, std::string>> meta;
    for (std::size_t i = 0; i < header.fields.size(); ++i) {
        const int width = i < header.size.size() ? header.size[i] : 1;
        const std::string type = i < header.type.size() ? header.type[i] : "f";
        meta[header.fields[i]] = {width, type};
    }

    const auto readField = [&](std::size_t at, const std::string& name, double& out) -> bool {
        const auto found = layout.offset.find(name);
        const auto info = meta.find(name);
        if (found == layout.offset.end() || info == meta.end()) return false;
        const std::size_t offset = at + static_cast<std::size_t>(found->second);
        const int width = info->second.first;
        const std::string& type = info->second.second;
        if (offset + static_cast<std::size_t>(width) > data.size()) return false;

        // Assemble the value most significant byte first; little endian stores it backwards.
        std::uint64_t raw = 0;
        for (int k = 0; k < width; ++k) {
            const std::size_t index = littleEndian ? data[offset + static_cast<std::size_t>(width - 1 - k)]
                                                   : data[offset + static_cast<std::size_t>(k)];
            raw = (raw << 8) | index;
        }
        if (type == "f" && width == 4) {
            const std::uint32_t bits = static_cast<std::uint32_t>(raw);
            float value = 0.0f;
            std::memcpy(&value, &bits, sizeof(float));
            out = static_cast<double>(value);
        } else if (type == "f" && width == 8) {
            double value = 0.0;
            std::memcpy(&value, &raw, sizeof(double));
            out = value;
        } else {
            // The integer types are widened as they are, without a sign flip, which is what a
            // plain DataView read gives.
            out = static_cast<double>(raw);
        }
        return true;
    };

    for (int i = 0; i < pointCount; ++i) {
        const std::size_t at = start + static_cast<std::size_t>(i) * static_cast<std::size_t>(layout.rowSize);
        double x = 0, y = 0, z = 0;
        if (readField(at, "x", x) && readField(at, "y", y) && readField(at, "z", z)) {
            points.positions.push_back(x);
            points.positions.push_back(y);
            points.positions.push_back(z);
        }
        double nx = 0, ny = 0, nz = 0;
        if (readField(at, "normal_x", nx) && readField(at, "normal_y", ny) &&
            readField(at, "normal_z", nz)) {
            points.normals.push_back(nx);
            points.normals.push_back(ny);
            points.normals.push_back(nz);
        }
        double rgb = 0;
        if (readField(at, "rgb", rgb)) {
            int channels[3] = {0, 0, 0};
            unpackRGB(static_cast<unsigned int>(rgb), channels);
            const std::vector<double> components{channels[0] / 255.0, channels[1] / 255.0,
                                                 channels[2] / 255.0};
            Color color;
            color.fromArray(components);
            ColorManagement::colorSpaceToWorking(color, SRGBColorSpace);
            points.colors.push_back(color.r);
            points.colors.push_back(color.g);
            points.colors.push_back(color.b);
        }
        double intensity = 0;
        if (readField(at, "intensity", intensity)) points.intensity.push_back(intensity);
        double label = 0;
        if (readField(at, "label", label)) points.label.push_back(label);
    }
    return points;
}

PCDLoader::Points PCDLoader::parseCompressedPoints(const std::vector<std::uint8_t>& data,
                                                   std::size_t start, int pointCount,
                                                   const Header& header, const FieldLayout& layout,
                                                   bool littleEndian) {
    if (start + 8 > data.size())
        throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");

    // Two little endian lengths precede the payload in this form.
    const auto readLength = [&data](std::size_t at) {
        return static_cast<std::size_t>(data[at]) | (static_cast<std::size_t>(data[at + 1]) << 8) |
               (static_cast<std::size_t>(data[at + 2]) << 16) |
               (static_cast<std::size_t>(data[at + 3]) << 24);
    };
    const std::size_t compressedSize = readLength(start);
    const std::size_t uncompressedSize = readLength(start + 4);
    if (start + 8 + compressedSize > data.size())
        throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");

    std::vector<std::uint8_t> compressed(data.begin() + static_cast<std::ptrdiff_t>(start + 8),
                                        data.begin() + static_cast<std::ptrdiff_t>(start + 8 + compressedSize));
    const auto plain = decompressLZF(compressed, uncompressedSize);
    // The decompressed block is plain binary from then on.
    return parseBinaryPoints(plain, 0, pointCount, header, layout, littleEndian);
}

std::shared_ptr<BufferGeometry> PCDLoader::buildGeometry(const Points& points) {
    auto geometry = std::make_shared<BufferGeometry>();
    // Each stream becomes an attribute only when it has data.
    if (!points.positions.empty())
        geometry->setAttribute("position",
                               std::make_shared<TypedBufferAttribute<float>>(points.positions, 3));
    if (!points.normals.empty())
        geometry->setAttribute("normal",
                               std::make_shared<TypedBufferAttribute<float>>(points.normals, 3));
    if (!points.colors.empty())
        geometry->setAttribute("color",
                               std::make_shared<TypedBufferAttribute<float>>(points.colors, 3));
    if (!points.intensity.empty())
        geometry->setAttribute("intensity",
                               std::make_shared<TypedBufferAttribute<float>>(points.intensity, 1));
    if (!points.label.empty())
        geometry->setAttribute("label",
                               std::make_shared<TypedBufferAttribute<float>>(points.label, 1));
    return geometry;
}