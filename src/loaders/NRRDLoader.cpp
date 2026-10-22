#include "loaders/NRRDLoader.h"

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

std::vector<std::string> splitWhitespace(const std::string& text) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) parts.push_back(token);
    return parts;
}
}  // namespace

NRRDLoader::Header NRRDLoader::parseHeader(const std::string& text) {
    Header header;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        std::string line = end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        start = end == std::string::npos ? text.size() + 1 : end + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // The magic line identifies the file.
        const std::size_t magic = line.find("NRRD");
        if (magic != std::string::npos && magic == line.find_first_not_of(" \t")) {
            header.isNrrd = true;
            continue;
        }
        const std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;

        // Every other line is a key and a value separated by the first colon.
        const std::size_t colon = trimmed.find(':');
        if (colon == std::string::npos) continue;
        const std::string key = trim(trimmed.substr(0, colon));
        const std::string value = trim(trimmed.substr(colon + 1));
        header.fields[key] = value;

        // A few keys get typed handling, matching the upstream field functions.
        if (key == "encoding") {
            header.encoding = value;
        } else if (key == "dimension") {
            header.dimension = std::atoi(value.c_str());
        } else if (key == "sizes") {
            header.sizes.clear();
            for (const auto& token : splitWhitespace(value))
                header.sizes.push_back(std::strtod(token.c_str(), nullptr));
        } else if (key == "spacings") {
            header.spacings.clear();
            for (const auto& token : splitWhitespace(value))
                header.spacings.push_back(std::strtod(token.c_str(), nullptr));
        }
    }

    if (!header.isNrrd) throw std::runtime_error("THREE.NRRDLoader: Not an NRRD file");
    // The bzip encodings need a decompressor the loader does not carry.
    if (header.encoding == "bz2" || header.encoding == "bzip2")
        throw std::runtime_error("THREE.NRRDLoader: Bzip is not supported");
    return header;
}

void NRRDLoader::resolveVectors(Header& header) {
    if (!header.vectors.empty()) return;

    // No space directions means the axes are the identity.
    header.vectors = {{{1, 0, 0}}, {{0, 1, 0}}, {{0, 0, 1}}};
    // Each declared spacing scales its own row.
    for (std::size_t i = 0; i < 3 && i < header.spacings.size(); ++i) {
        const double spacing = header.spacings[i];
        if (spacing != spacing) continue;  // a not a number entry is ignored
        for (std::size_t j = 0; j < 3; ++j) header.vectors[i][j] *= spacing;
    }
}
