#include "loaders/VTKLoader.h"

#include "math/Color.h"

#include <cmath>
#include <stdexcept>
#include <cstdlib>

std::size_t VTKLoader::wordLength(const std::string& line) {
    // Everything before the first character that can begin or continue a number.
    std::size_t at = 0;
    while (at < line.size()) {
        const char c = line[at];
        const bool isDigit = c >= '0' && c <= '9';
        const bool isNumberish = isDigit || c == '.' || c == '-' || c == ' ' || c == '\t' ||
                                 c == '\n' || c == '\r' || c == '\f' || c == '\v';
        if (isNumberish) break;
        ++at;
    }
    return at;
}

std::vector<double> VTKLoader::parseFloats(const std::string& line) {
    std::vector<double> values;
    std::size_t at = 0;
    while (at < line.size()) {
        // Skip the whitespace that separates tokens.
        const char c = line[at];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') {
            ++at;
            continue;
        }
        // Take the token up to the next whitespace.
        std::size_t end = at;
        while (end < line.size()) {
            const char e = line[end];
            if (e == ' ' || e == '\t' || e == '\n' || e == '\r' || e == '\f' || e == '\v') break;
            ++end;
        }
        const std::string token = line.substr(at, end - at);
        at = end;

        // The conversion reads as much of the token as it can; a token that begins with
        // something else is not a number at all and reads as NaN.
        char* stop = nullptr;
        const double value = std::strtod(token.c_str(), &stop);
        if (stop == token.c_str()) {
            values.push_back(std::nan(""));
        } else {
            values.push_back(value);
        }
    }
    return values;
}

bool VTKLoader::isNumberLine(const std::string& line) {
    // Upstream trims the line before testing it, so leading whitespace is skipped here.
    std::size_t at = 0;
    while (at < line.size() && (line[at] == ' ' || line[at] == '\t' ||
                                line[at] == '\r' || line[at] == '\n')) {
        ++at;
    }
    // A line that begins with a word is a keyword line; one that begins with a number
    // is not.
    return wordLength(line.substr(at)) == 0;
}

bool VTKLoader::parseConnectivity(const std::string& line, Connectivity& out) {
    out.numVertices = 0;
    out.indices.clear();

    // The line has to begin with a count, so leading whitespace is skipped first.
    std::size_t at = 0;
    while (at < line.size() && (line[at] == ' ' || line[at] == '\t')) ++at;
    if (at >= line.size() || line[at] < '0' || line[at] > '9') return false;

    // The count runs to the end of the digits.
    std::size_t end = at;
    while (end < line.size() && line[end] >= '0' && line[end] <= '9') ++end;
    const std::string count = line.substr(at, end - at);
    if (count.empty()) return false;
    out.numVertices = std::atoi(count.c_str());

    // Everything after the count is read as integers; anything that is not a number ends the
    // list, which is what the upstream pattern effectively does by only allowing digits.
    at = end;
    while (at < line.size()) {
        const char c = line[at];
        if (c < '0' || c > '9') { ++at; continue; }
        std::size_t stop = at;
        while (stop < line.size() && line[stop] >= '0' && line[stop] <= '9') ++stop;
        out.indices.push_back(std::atoi(line.substr(at, stop - at).c_str()));
        at = stop;
    }
    return true;
}

std::vector<int> VTKLoader::triangulateFan(const std::vector<int>& indices, int numVertices) {
    std::vector<int> triangles;
    // Fewer than three nodes cannot form a triangle.
    if (numVertices < 3) return triangles;

    // Every triangle shares the first node.
    const int first = indices.empty() ? 0 : indices[0];
    int k = 1;
    for (int j = 0; j < numVertices - 2; ++j) {
        if (static_cast<std::size_t>(k + 1) >= indices.size()) break;
        const int second = indices[static_cast<std::size_t>(k)];
        const int third = indices[static_cast<std::size_t>(k + 1)];
        triangles.push_back(first);
        triangles.push_back(second);
        triangles.push_back(third);
        ++k;
    }
    return triangles;
}

void VTKLoader::applyKeyword(const std::string& line, Sections& sections) {
    // Every keyword is its word followed by a space, so a bare word does not count.
    const auto has = [&line](const char* keyword) {
        const std::string prefix(keyword);
        return line.compare(0, prefix.size(), prefix) == 0;
    };

    // The order matters only in that the first match wins, as it does upstream.
    if (has("POLYGONS ")) {
        sections.polygons = true;
        sections.points = false;
        sections.triangleStrips = false;
    } else if (has("POINTS ")) {
        sections.polygons = false;
        sections.points = true;
        sections.triangleStrips = false;
    } else if (has("TRIANGLE_STRIPS ")) {
        sections.polygons = false;
        sections.points = false;
        sections.triangleStrips = true;
    } else if (has("POINT_DATA ")) {
        // Note that the normals and colours flags are left as they were.
        sections.pointData = true;
        sections.points = false;
        sections.polygons = false;
        sections.triangleStrips = false;
    } else if (has("CELL_DATA ")) {
        sections.cellData = true;
        sections.points = false;
        sections.polygons = false;
        sections.triangleStrips = false;
    } else if (has("COLOR_SCALARS ")) {
        sections.colors = true;
        sections.normals = false;
        sections.points = false;
        sections.polygons = false;
        sections.triangleStrips = false;
    } else if (has("NORMALS ")) {
        sections.normals = true;
        sections.colors = false;
        sections.points = false;
        sections.polygons = false;
        sections.triangleStrips = false;
    }
}

VTKLoader::Mesh VTKLoader::parseAscii(const std::string& text) {
    Mesh mesh;
    Sections sections;

    // The file is read line by line.
    std::size_t at = 0;
    while (at <= text.size()) {
        std::size_t end = text.find('\n', at);
        if (end == std::string::npos) end = text.size();
        // The line is trimmed before anything looks at it.
        std::string line = text.substr(at, end - at);
        std::size_t begin = 0;
        while (begin < line.size() && (line[begin] == ' ' || line[begin] == '\t' ||
                                       line[begin] == '\r')) {
            ++begin;
        }
        std::size_t last = line.size();
        while (last > begin && (line[last - 1] == ' ' || line[last - 1] == '\t' ||
                                line[last - 1] == '\r')) {
            --last;
        }
        line = line.substr(begin, last - begin);
        at = end + 1;
        if (end == text.size()) break;

        // A dataset line names the kind of data; only polygonal data is understood.
        if (line.compare(0, 7, "DATASET") == 0) {
            const std::size_t space = line.find(' ');
            const std::string dataset = space == std::string::npos ? std::string()
                                                                   : line.substr(space + 1);
            if (dataset != "POLYDATA") {
                throw std::runtime_error("Unsupported DATASET type: " + dataset);
            }
            continue;
        }

        // The section handling comes before the keyword handling, and the branches are
        // exclusive, so a keyword line met inside a section is dealt with as that section sees
        // it rather than as the section it opens.
        if (sections.points) {
            if (isNumberLine(line)) {
                const auto values = parseFloats(line);
                // Only whole triples count.
                for (std::size_t k = 0; k + 2 < values.size(); k += 3) {
                    mesh.positions.push_back(values[k]);
                    mesh.positions.push_back(values[k + 1]);
                    mesh.positions.push_back(values[k + 2]);
                }
            }
        } else if (sections.polygons) {
            Connectivity connectivity;
            if (parseConnectivity(line, connectivity)) {
                const auto triangles = triangulateFan(connectivity.indices, connectivity.numVertices);
                mesh.indices.insert(mesh.indices.end(), triangles.begin(), triangles.end());
            }
        }

        // Every line then possibly opens a new section.
        applyKeyword(line, sections);
    }
    return mesh;
}

std::vector<double> VTKLoader::parseColors(const std::string& line) {
    std::vector<double> colors;
    const auto values = parseFloats(line);
    for (std::size_t k = 0; k + 2 < values.size(); k += 3) {
        // The colour helper does the display to working space conversion for each channel, so
        // the values that come out are not the ones in the file.
        Color color;
        color.setRGB(static_cast<HIGH_PRECISION>(values[k]),
                     static_cast<HIGH_PRECISION>(values[k + 1]),
                     static_cast<HIGH_PRECISION>(values[k + 2]), SRGBColorSpace);
        colors.push_back(static_cast<double>(color.r));
        colors.push_back(static_cast<double>(color.g));
        colors.push_back(static_cast<double>(color.b));
    }
    return colors;
}

std::vector<double> VTKLoader::parseNormals(const std::string& line) {
    std::vector<double> normals;
    const auto values = parseFloats(line);
    // Normals are copied as they are, with no conversion of any kind.
    for (std::size_t k = 0; k + 2 < values.size(); k += 3) {
        normals.push_back(values[k]);
        normals.push_back(values[k + 1]);
        normals.push_back(values[k + 2]);
    }
    return normals;
}