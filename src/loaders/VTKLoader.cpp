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
