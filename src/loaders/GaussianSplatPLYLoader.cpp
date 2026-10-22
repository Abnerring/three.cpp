#include "loaders/GaussianSplatPLYLoader.h"

#include <cctype>
#include <sstream>

namespace {
// The property names a splat file has to declare, beyond the position.
const std::vector<std::string> kRequired{"x",  "y",  "z",  "scale_0", "scale_1", "scale_2",
                                         "rot_0", "rot_1", "rot_2", "rot_3",
                                         "f_dc_0", "f_dc_1", "f_dc_2", "opacity"};
}  // namespace

GaussianSplatPLYLoader::Header GaussianSplatPLYLoader::scanHeader(const std::string& text) {
    Header header;

    // The header runs from the magic word to the end marker.
    const std::size_t magic = text.find("ply");
    if (magic == std::string::npos) return header;
    const std::size_t end = text.find("end_header");
    if (end == std::string::npos) return header;
    header.isPly = true;

    std::istringstream stream(text.substr(magic, end - magic));
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // A property line names a type and then the property itself.
        std::istringstream words(line);
        std::string first, type, name;
        if (!(words >> first)) continue;
        if (first != "property") continue;
        if (!(words >> type)) continue;
        // A list property names a count type before the type, which a splat file does not use.
        std::string extra;
        if (words >> name) {
            if (words >> extra) {
                name = extra;
            }
        }
        if (name.empty()) continue;
        header.properties.push_back(name);

        // The spherical harmonics properties are the ones counted, and their number fixes the
        // degree.
        if (name.rfind("f_rest_", 0) == 0) ++header.restComponents;
    }

    // A count that matches an entry of the table names that degree.
    for (int degree = 0; degree < 4; ++degree)
        if (header.restComponents == kDegreeComponents[degree]) header.degree = degree;

    // Report which required names are absent.
    for (const auto& required : kRequired) {
        bool found = false;
        for (const auto& property : header.properties)
            if (property == required) {
                found = true;
                break;
            }
        if (!found) header.missing.push_back(required);
    }
    return header;
}
