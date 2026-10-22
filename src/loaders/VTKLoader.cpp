#include "loaders/VTKLoader.h"

#include <cmath>
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
