#include "loaders/NRRDLoader.h"

#include <array>
#include <cctype>
#include <cstring>
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

// Reads a space directions value: per axis either a parenthesised triple or a word such as
// none, which leaves that axis as the identity row.
std::vector<std::array<double, 3>> parseDirections(const std::string& value) {
    std::vector<std::array<double, 3>> out;
    std::size_t i = 0;
    while (i < value.size() && out.size() < 3) {
        while (i < value.size() && std::isspace(static_cast<unsigned char>(value[i]))) ++i;
        if (i >= value.size()) break;
        if (value[i] == '(') {
            const std::size_t close = value.find(')', i);
            if (close == std::string::npos) break;
            std::array<double, 3> triple{0, 0, 0};
            std::size_t part = 0;
            std::size_t at = i + 1;
            while (at < close && part < 3) {
                const std::size_t comma = value.find(',', at);
                const std::size_t end = (comma == std::string::npos || comma > close) ? close : comma;
                if (end <= at) break;
                triple[part++] = std::strtod(value.substr(at, end - at).c_str(), nullptr);
                at = end + 1;
            }
            out.push_back(triple);
            i = close + 1;
        } else {
            const std::size_t space = value.find(' ', i);
            const std::size_t stop = space == std::string::npos ? value.size() : space;
            const std::string word = value.substr(i, stop - i);
            // A word such as none leaves this axis pointing along itself.
            std::array<double, 3> row{0, 0, 0};
            row[out.size() < 3 ? out.size() : 2] = 1.0;
            out.push_back(row);
            (void)word;
            i = stop + 1;
        }
    }
    return out;
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
        } else if (key == "space directions") {
            // Explicit directions replace the identity, so the spacings do not apply.
            header.vectors = parseDirections(value);
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

std::vector<double> NRRDLoader::parseTextData(const std::string& text,
                                              const std::vector<double>& sizes, bool floating,
                                              bool hex) {
    // The body holds one value per voxel, so the count is the product of the sizes.
    std::size_t expected = 1;
    for (const double size : sizes) {
        if (size <= 0) continue;
        expected *= static_cast<std::size_t>(size);
    }

    std::vector<double> out;
    out.reserve(expected);
    std::string number;

    const auto flush = [&]() {
        if (number.empty()) return;
        if (out.size() < expected) {
            // A floating point type keeps the fraction, an integer type stops at the point.
            if (floating) out.push_back(std::strtod(number.c_str(), nullptr));
            else out.push_back(static_cast<double>(std::strtoll(number.c_str(), nullptr, hex ? 16 : 10)));
        }
        number.clear();
    };

    for (const char character : text) {
        const int code = static_cast<unsigned char>(character);
        // Tab through carriage return and the space separate numbers; everything else joins.
        if ((code < 9 || code > 13) && code != 32) number.push_back(character);
        else flush();
    }
    flush();
    return out;
}

int NRRDLoader::typeWidth(const std::string& type) {
    if (type == "uchar" || type == "schar" || type == "char") return 1;
    if (type == "ushort" || type == "sshort" || type == "short") return 2;
    if (type == "uint" || type == "sint" || type == "int" || type == "float") return 4;
    if (type == "double") return 8;
    return 0;
}

std::vector<double> NRRDLoader::parseBinaryData(const std::vector<std::uint8_t>& data,
                                                std::size_t start, std::size_t count,
                                                const std::string& type) {
    const int width = typeWidth(type);
    if (width == 0) throw std::runtime_error("THREE.NRRDLoader: Unknown type " + type);

    std::vector<double> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t at = start + i * static_cast<std::size_t>(width);
        if (at + static_cast<std::size_t>(width) > data.size()) break;

        // Values assemble most significant byte first; the body is little endian, so the
        // bytes are read backwards.
        std::uint64_t raw = 0;
        for (int k = 0; k < width; ++k)
            raw = (raw << 8) | data[at + static_cast<std::size_t>(width - 1 - k)];

        if (type == "uchar" || type == "char") out.push_back(static_cast<double>(raw));
        else if (type == "schar") out.push_back(static_cast<double>(static_cast<std::int8_t>(raw)));
        else if (type == "ushort" || type == "short") out.push_back(static_cast<double>(raw));
        else if (type == "sshort") out.push_back(static_cast<double>(static_cast<std::int16_t>(raw)));
        else if (type == "uint" || type == "int") out.push_back(static_cast<double>(raw));
        else if (type == "sint") out.push_back(static_cast<double>(static_cast<std::int32_t>(raw)));
        else if (type == "float") {
            const std::uint32_t bits = static_cast<std::uint32_t>(raw);
            float value = 0.0f;
            std::memcpy(&value, &bits, sizeof(float));
            out.push_back(static_cast<double>(value));
        } else {
            double value = 0.0;
            std::memcpy(&value, &raw, sizeof(double));
            out.push_back(value);
        }
    }
    return out;
}