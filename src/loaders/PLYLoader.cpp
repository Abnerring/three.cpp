#include "loaders/PLYLoader.h"

#include <cstdlib>
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