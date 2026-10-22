#include "loaders/PLYLoader.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::vector<std::string> splitWhitespace(const std::string& text) {
    std::vector<std::string> parts;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        if (i >= text.size()) break;
        const std::size_t start = i;
        while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        parts.push_back(text.substr(start, i - start));
    }
    return parts;
}
}  // namespace

PLYLoader::Header PLYLoader::parseHeader(const std::string& text) const {
    // Upstream matches ply then everything up to end_header, so the magic has to be present.
    if (text.rfind("ply", 0) != 0)
        throw std::runtime_error("THREE.PLYLoader: Missing ply header");
    const std::size_t endHeader = text.find("end_header");
    if (endHeader == std::string::npos)
        throw std::runtime_error("THREE.PLYLoader: Missing ply header terminator");

    Header header;
    // The block starts right after the three magic characters.
    const std::string block = text.substr(3, endHeader - 3);

    bool hasElement = false;
    Element current;

    std::size_t start = 0;
    while (start <= block.size()) {
        const std::size_t stop = block.find('\n', start);
        const std::string raw = stop == std::string::npos ? block.substr(start) : block.substr(start, stop - start);
        start = stop == std::string::npos ? block.size() + 1 : stop + 1;

        const std::string line = trim(raw);
        if (line.empty()) continue;

        auto values = splitWhitespace(line);
        if (values.empty()) continue;
        const std::string lineType = values.front();
        values.erase(values.begin());
        const std::string rest = [&values] {
            std::string joined;
            for (std::size_t i = 0; i < values.size(); ++i) {
                if (i) joined += ' ';
                joined += values[i];
            }
            return joined;
        }();

        if (lineType == "format") {
            if (!values.empty()) header.format = values[0];
            if (values.size() > 1) header.version = values[1];
        } else if (lineType == "comment") {
            header.comments.push_back(rest);
        } else if (lineType == "element") {
            if (hasElement) header.elements.push_back(current);
            current = Element{};
            if (!values.empty()) current.name = values[0];
            if (values.size() > 1) current.count = std::atoi(values[1].c_str());
            hasElement = true;
        } else if (lineType == "property") {
            Property property;
            if (values.empty()) continue;
            property.type = values[0];
            if (property.isList()) {
                // list countType itemType name
                if (values.size() > 1) property.countType = values[1];
                if (values.size() > 2) property.itemType = values[2];
                if (values.size() > 3) property.name = values[3];
            } else {
                if (values.size() > 1) property.name = values[1];
            }
            const auto renamed = propertyNameMapping.find(property.name);
            if (renamed != propertyNameMapping.end()) property.name = renamed->second;
            current.properties.push_back(property);
        } else if (lineType == "obj_info") {
            header.objInfo = rest;
        }
        // Any other line type is ignored, as upstream only logs it.
    }
    if (hasElement) header.elements.push_back(current);
    return header;
}

std::string PLYLoader::bufferAttributeClass(const std::string& type) {
    // Both spellings map to the same class, exactly as the upstream switch does.
    if (type == "int8" || type == "char") return "Int8BufferAttribute";
    if (type == "uint8" || type == "uchar") return "Uint8BufferAttribute";
    if (type == "int16" || type == "short") return "Int16BufferAttribute";
    if (type == "uint16" || type == "ushort") return "Uint16BufferAttribute";
    if (type == "int32" || type == "int") return "Int32BufferAttribute";
    if (type == "uint32" || type == "uint") return "Uint32BufferAttribute";
    if (type == "float32" || type == "float") return "Float32BufferAttribute";
    if (type == "float64" || type == "double") return "Float64BufferAttribute";
    return {};
}

double PLYLoader::colorScale(const std::string& type) {
    if (type == "uchar" || type == "uint8") return 1.0 / 255.0;
    if (type == "ushort" || type == "uint16") return 1.0 / 65535.0;
    if (type == "float" || type == "float32" || type == "double" || type == "float64") return 1.0;
    // Anything else falls back to the byte width.
    return 1.0 / 255.0;
}

bool PLYLoader::isIntegerType(const std::string& type) {
    return type == "char" || type == "uchar" || type == "short" || type == "ushort" ||
           type == "int" || type == "uint" || type == "int8" || type == "uint8" ||
           type == "int16" || type == "uint16" || type == "int32" || type == "uint32";
}

double PLYLoader::parseASCIINumber(const std::string& text, const std::string& type) {
    // Integer spellings are parsed as integers and everything else as a float, which is what
    // upstream's two case groups do. A type in neither group yields zero here.
    if (isIntegerType(type)) return static_cast<double>(std::atoll(text.c_str()));
    if (type == "float" || type == "double" || type == "float32" || type == "float64")
        return std::strtod(text.c_str(), nullptr);
    return 0.0;
}

std::optional<PLYLoader::ElementRecord> PLYLoader::parseASCIIElement(
    const std::vector<Property>& properties, const std::vector<std::string>& tokens,
    std::size_t& cursor) {
    ElementRecord element;
    for (const auto& property : properties) {
        if (cursor >= tokens.size()) return std::nullopt;

        if (property.isList()) {
            const double countValue = parseASCIINumber(tokens[cursor++], property.countType);
            const auto count = static_cast<std::size_t>(countValue < 0 ? 0 : countValue);
            std::vector<double> list;
            list.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                if (cursor >= tokens.size()) return std::nullopt;
                list.push_back(parseASCIINumber(tokens[cursor++], property.itemType));
            }
            element[property.name] = std::move(list);
        } else {
            element[property.name] = parseASCIINumber(tokens[cursor++], property.type);
        }
    }
    return element;
}

int PLYLoader::binaryTypeSize(const std::string& type) {
    if (type == "int8" || type == "char" || type == "uint8" || type == "uchar") return 1;
    if (type == "int16" || type == "short" || type == "uint16" || type == "ushort") return 2;
    if (type == "int32" || type == "int" || type == "uint32" || type == "uint" ||
        type == "float32" || type == "float")
        return 4;
    if (type == "float64" || type == "double") return 8;
    return 0;
}

namespace {
double readBinaryScalar(const std::vector<std::uint8_t>& data, std::size_t at,
                        const std::string& type, bool littleEndian, bool& ok) {
    const int size = PLYLoader::binaryTypeSize(type);
    if (size == 0 || at + static_cast<std::size_t>(size) > data.size()) {
        ok = false;
        return 0.0;
    }
    // The value is assembled most significant byte first. Little endian stores the least
    // significant byte at the lowest address, so it is read backwards.
    std::uint64_t raw = 0;
    for (int i = 0; i < size; ++i) {
        const std::size_t index = littleEndian ? data[at + static_cast<std::size_t>(size - 1 - i)]
                                               : data[at + static_cast<std::size_t>(i)];
        raw = (raw << 8) | index;
    }
    const auto bitsFor = [&](auto sample) {
        using Sample = decltype(sample);
        Sample value{};
        if (size <= static_cast<int>(sizeof(Sample))) {
            std::memcpy(&value, &raw, static_cast<std::size_t>(size));
        }
        return value;
    };
    (void)bitsFor;

    // Sign extend the integer widths before converting.
    const auto signedOf = [&](int bits) -> double {
        const std::uint64_t mask = (bits == 64) ? ~std::uint64_t(0) : ((std::uint64_t(1) << bits) - 1);
        std::uint64_t value = raw & mask;
        const std::uint64_t signBit = std::uint64_t(1) << (bits - 1);
        if (value & signBit) {
            // Two's complement into a signed 64 bit value.
            value |= ~mask;
            return static_cast<double>(static_cast<std::int64_t>(value));
        }
        return static_cast<double>(value);
    };

    if (type == "int8" || type == "char") return signedOf(8);
    if (type == "uint8" || type == "uchar") return static_cast<double>(raw & 0xff);
    if (type == "int16" || type == "short") return signedOf(16);
    if (type == "uint16" || type == "ushort") return static_cast<double>(raw & 0xffff);
    if (type == "int32" || type == "int") return signedOf(32);
    if (type == "uint32" || type == "uint") return static_cast<double>(raw & 0xffffffffull);
    if (type == "float32" || type == "float") {
        const std::uint32_t bits = static_cast<std::uint32_t>(raw & 0xffffffffull);
        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(float));
        return static_cast<double>(value);
    }
    if (type == "float64" || type == "double") {
        double value = 0.0;
        std::memcpy(&value, &raw, sizeof(double));
        return value;
    }
    ok = false;
    return 0.0;
}
}  // namespace

std::optional<std::pair<PLYLoader::ElementRecord, std::size_t>> PLYLoader::binaryReadElement(
    const std::vector<std::uint8_t>& data, std::size_t at, const std::vector<Property>& properties,
    bool littleEndian) {
    ElementRecord element;
    std::size_t read = 0;
    bool ok = true;

    for (const auto& property : properties) {
        if (property.isList()) {
            const double countValue =
                readBinaryScalar(data, at + read, property.countType, littleEndian, ok);
            read += static_cast<std::size_t>(binaryTypeSize(property.countType));
            if (!ok) return std::nullopt;

            const auto count = static_cast<std::size_t>(countValue < 0 ? 0 : countValue);
            std::vector<double> list;
            list.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                list.push_back(readBinaryScalar(data, at + read, property.itemType, littleEndian, ok));
                read += static_cast<std::size_t>(binaryTypeSize(property.itemType));
                if (!ok) return std::nullopt;
            }
            element[property.name] = std::move(list);
        } else {
            const double value = readBinaryScalar(data, at + read, property.type, littleEndian, ok);
            read += static_cast<std::size_t>(binaryTypeSize(property.type));
            if (!ok) return std::nullopt;
            element[property.name] = value;
        }
    }
    return std::make_pair(std::move(element), read);
}