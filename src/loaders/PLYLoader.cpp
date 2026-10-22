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
