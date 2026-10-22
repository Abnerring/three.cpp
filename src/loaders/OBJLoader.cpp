#include "loaders/OBJLoader.h"

#include "math/Color.h"
#include "math/ColorManagement.h"

#include <cstdlib>
#include <sstream>

namespace {
std::string trimStart(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    return first == std::string::npos ? std::string() : text.substr(first);
}

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

std::vector<std::string> splitSlash(const std::string& text) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t slash = text.find('/', start);
        if (slash == std::string::npos) {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, slash - start));
        start = slash + 1;
    }
    return parts;
}

// Upstream: ( index >= 0 ? index - 1 : index + length ) * scale, where length is the flat
// array length and scale is what the caller uses for that kind.
int resolveIndex(const std::string& value, std::size_t length, int scale, bool relativeBase,
                 std::size_t itemsPerEntry) {
    if (value.empty()) return 0;
    const int index = std::atoi(value.c_str());
    const int relative = static_cast<int>(length / itemsPerEntry);
    return (index >= 0 ? index - 1 : index + relative) * scale;
}
}  // namespace

int OBJLoader::parseVertexIndex(const std::string& value, std::size_t length) {
    return resolveIndex(value, length, 3, true, 3);
}

int OBJLoader::parseNormalIndex(const std::string& value, std::size_t length) {
    return resolveIndex(value, length, 3, true, 3);
}

int OBJLoader::parseUVIndex(const std::string& value, std::size_t length) {
    return resolveIndex(value, length, 2, true, 2);
}

OBJLoader::Geometry OBJLoader::parseGeometry(const std::string& text) const {
    std::string normalised = text;
    // Carriage returns are normalised before anything else, then line continuations are
    // joined so a wrapped statement reads as one line.
    if (normalised.find("\r\n") != std::string::npos) {
        std::string replaced;
        for (std::size_t i = 0; i < normalised.size(); ++i) {
            if (normalised[i] == '\r' && i + 1 < normalised.size() && normalised[i + 1] == '\n')
                continue;  // drop the carriage return
            replaced.push_back(normalised[i]);
        }
        normalised = replaced;
    }
    if (normalised.find("\\\n") != std::string::npos) {
        std::string replaced;
        for (std::size_t i = 0; i < normalised.size(); ++i) {
            // Both the backslash and the newline after it are removed, so the two halves of
            // the statement end up on one line.
            if (normalised[i] == '\\' && i + 1 < normalised.size() && normalised[i + 1] == '\n') {
                ++i;
                continue;
            }
            replaced.push_back(normalised[i]);
        }
        normalised = replaced;
    }

    Geometry result;
    std::size_t start = 0;
    while (start <= normalised.size()) {
        const std::size_t end = normalised.find('\n', start);
        const std::string raw =
            end == std::string::npos ? normalised.substr(start) : normalised.substr(start, end - start);
        start = end == std::string::npos ? normalised.size() + 1 : end + 1;

        const std::string line = trimStart(raw);
        if (line.empty() || line[0] == '#') continue;

        const char first = line[0];
        if (first == 'v') {
            const auto data = splitWhitespace(line);
            if (data.empty()) continue;
            if (data[0] == "v") {
                if (data.size() < 4) continue;
                result.vertices.push_back(std::strtod(data[1].c_str(), nullptr));
                result.vertices.push_back(std::strtod(data[2].c_str(), nullptr));
                result.vertices.push_back(std::strtod(data[3].c_str(), nullptr));
                if (data.size() >= 7) {
                    // A vertex colour is given in sRGB and converted to the working space.
                    const std::vector<double> components{std::strtod(data[4].c_str(), nullptr),
                                                         std::strtod(data[5].c_str(), nullptr),
                                                         std::strtod(data[6].c_str(), nullptr)};
                    Color color;
                    color.fromArray(components);
                    ColorManagement::colorSpaceToWorking(color, SRGBColorSpace);
                    result.colors.push_back(color.r);
                    result.colors.push_back(color.g);
                    result.colors.push_back(color.b);
                    result.hasVertexColors = true;
                } else {
                    // Placeholders keep the colour and vertex streams the same length.
                    result.colors.push_back(0.0);
                    result.colors.push_back(0.0);
                    result.colors.push_back(0.0);
                }
            } else if (data[0] == "vn") {
                if (data.size() < 4) continue;
                result.normals.push_back(std::strtod(data[1].c_str(), nullptr));
                result.normals.push_back(std::strtod(data[2].c_str(), nullptr));
                result.normals.push_back(std::strtod(data[3].c_str(), nullptr));
            } else if (data[0] == "vt") {
                if (data.size() < 2) continue;
                result.uvs.push_back(std::strtod(data[1].c_str(), nullptr));
                // A missing second component is zero, as a not a number parse would give.
                result.uvs.push_back(data.size() > 2 ? std::strtod(data[2].c_str(), nullptr) : 0.0);
            }
        } else if (first == 'f') {
            const auto vertexData = splitWhitespace(trim(line.substr(1)));
            std::vector<std::vector<std::string>> faceVertices;
            for (const auto& vertex : vertexData)
                if (!vertex.empty()) faceVertices.push_back(splitSlash(vertex));
            if (faceVertices.size() < 3) continue;

            // The first corner fans out to every later pair, forming an n-gon.
            const auto& v1 = faceVertices[0];
            for (std::size_t j = 1; j + 1 < faceVertices.size(); ++j) {
                const auto& v2 = faceVertices[j];
                const auto& v3 = faceVertices[j + 1];
                const auto at = [](const std::vector<std::string>& parts, std::size_t index) {
                    return index < parts.size() ? parts[index] : std::string();
                };
                Triangle triangle{};
                triangle[0] = parseVertexIndex(at(v1, 0), result.vertices.size());
                triangle[1] = parseVertexIndex(at(v2, 0), result.vertices.size());
                triangle[2] = parseVertexIndex(at(v3, 0), result.vertices.size());
                triangle[3] = parseNormalIndex(at(v1, 2), result.normals.size());
                triangle[4] = parseNormalIndex(at(v2, 2), result.normals.size());
                triangle[5] = parseNormalIndex(at(v3, 2), result.normals.size());
                triangle[6] = parseUVIndex(at(v1, 1), result.uvs.size());
                triangle[7] = parseUVIndex(at(v2, 1), result.uvs.size());
                triangle[8] = parseUVIndex(at(v3, 1), result.uvs.size());
                result.triangles.push_back(triangle);
            }
        }
    }
    return result;
}
