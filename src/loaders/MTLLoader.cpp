#include "loaders/MTLLoader.h"

#include <cctype>
#include <cstdlib>
#include <vector>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string lower(std::string text) {
    for (auto& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::vector<std::string> splitWhitespace(const std::string& text, std::size_t limit) {
    std::vector<std::string> parts;
    std::size_t i = 0;
    while (i < text.size() && parts.size() < limit) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        if (i >= text.size()) break;
        const std::size_t start = i;
        while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        parts.push_back(text.substr(start, i - start));
    }
    return parts;
}
}  // namespace

MTLLoader::MaterialsInfo MTLLoader::parse(const std::string& text) const {
    MaterialsInfo materials;
    MaterialInfo* current = nullptr;

    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        const std::string raw =
            end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        start = end == std::string::npos ? text.size() + 1 : end + 1;

        const std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;

        const std::size_t pos = line.find(' ');
        const std::string key = lower(pos == std::string::npos ? line : line.substr(0, pos));
        const std::string value = trim(pos == std::string::npos ? std::string() : line.substr(pos + 1));

        if (key == "newmtl") {
            // A new material record starts here and becomes the target of later lines.
            current = &materials[value];
            (*current)["name"] = value;
            continue;
        }

        if (current == nullptr) continue;  // a property before any newmtl has no home

        if (key == "ka" || key == "kd" || key == "ks" || key == "ke") {
            const auto parts = splitWhitespace(value, 3);
            Color color{0.0, 0.0, 0.0};
            for (std::size_t i = 0; i < parts.size() && i < 3; ++i)
                color[i] = std::strtod(parts[i].c_str(), nullptr);
            (*current)[key] = color;
        } else {
            (*current)[key] = value;
        }
    }
    return materials;
}

MTLLoader::MaterialsInfo MTLLoader::convert(const MaterialsInfo& materialsInfo,
                                            const MaterialOptions& options) {
    MaterialsInfo converted;
    for (const auto& [name, info] : materialsInfo) {
        MaterialInfo covmat;
        for (const auto& [prop, raw] : info) {
            const std::string lprop = lower(prop);
            bool save = true;
            Value value = raw;

            // Only the three colour keys take part in the option handling upstream.
            if (lprop == "kd" || lprop == "ka" || lprop == "ks") {
                if (const auto* color = std::get_if<Color>(&value)) {
                    Color scaled = *color;
                    if (options.normalizeRGB)
                        scaled = {scaled[0] / 255.0, scaled[1] / 255.0, scaled[2] / 255.0};
                    if (options.ignoreZeroRGBs && scaled[0] == 0 && scaled[1] == 0 && scaled[2] == 0)
                        save = false;
                    value = scaled;
                }
            }

            if (save) covmat[lprop] = value;
        }
        converted[name] = covmat;
    }
    return converted;
}