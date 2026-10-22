#include "loaders/MTLLoader.h"

#include "math/ColorManagement.h"
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

std::vector<std::string> splitWhitespaceAll(const std::string& text) {
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
            ColorValue color{0.0, 0.0, 0.0};
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
                if (const auto* color = std::get_if<ColorValue>(&value)) {
                    ColorValue scaled = *color;
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

MTLLoader::TextureParams MTLLoader::getTextureParams(const std::string& value) {
    TextureParams result;
    std::vector<std::string> items = splitWhitespaceAll(value);

    const auto indexOf = [&items](const std::string& token) -> std::ptrdiff_t {
        for (std::size_t i = 0; i < items.size(); ++i)
            if (items[i] == token) return std::ptrdiff_t(i);
        return -1;
    };
    const auto erase = [&items](std::size_t at, std::size_t count) {
        if (at >= items.size()) return;
        const std::size_t end = std::min(items.size(), at + count);
        items.erase(items.begin() + std::ptrdiff_t(at), items.begin() + std::ptrdiff_t(end));
    };

    std::ptrdiff_t pos = indexOf("-bm");
    if (pos >= 0) {
        if (std::size_t(pos) + 1 < items.size())
            result.bumpScale = std::strtod(items[std::size_t(pos) + 1].c_str(), nullptr);
        erase(std::size_t(pos), 2);
    }

    pos = indexOf("-mm");
    if (pos >= 0) {
        if (std::size_t(pos) + 2 < items.size()) {
            result.displacementBias = std::strtod(items[std::size_t(pos) + 1].c_str(), nullptr);
            result.displacementScale = std::strtod(items[std::size_t(pos) + 2].c_str(), nullptr);
        }
        erase(std::size_t(pos), 3);
    }

    pos = indexOf("-s");
    if (pos >= 0) {
        if (std::size_t(pos) + 2 < items.size()) {
            result.scaleU = std::strtod(items[std::size_t(pos) + 1].c_str(), nullptr);
            result.scaleV = std::strtod(items[std::size_t(pos) + 2].c_str(), nullptr);
        }
        // Upstream removes four tokens here even though only two values are read.
        erase(std::size_t(pos), 4);
    }

    pos = indexOf("-o");
    if (pos >= 0) {
        if (std::size_t(pos) + 2 < items.size()) {
            result.offsetU = std::strtod(items[std::size_t(pos) + 1].c_str(), nullptr);
            result.offsetV = std::strtod(items[std::size_t(pos) + 2].c_str(), nullptr);
        }
        erase(std::size_t(pos), 4);
    }

    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i) result.url += ' ';
        result.url += items[i];
    }
    return result;
}

MTLLoader::MaterialParams MTLLoader::createMaterialParams(const MaterialInfo& materialInfo,
                                                        const MaterialOptions& options) {
    MaterialParams params;
    // Keeps the first texture seen for a slot, exactly like the upstream guard.
    const auto setMapForType = [&params](const std::string& slot, const std::string& value) {
        if (slot == "map") { if (params.map.empty()) params.map = value; }
        else if (slot == "specularMap") { if (params.specularMap.empty()) params.specularMap = value; }
        else if (slot == "emissiveMap") { if (params.emissiveMap.empty()) params.emissiveMap = value; }
        else if (slot == "normalMap") { if (params.normalMap.empty()) params.normalMap = value; }
        else if (slot == "bumpMap") { if (params.bumpMap.empty()) params.bumpMap = value; }
        else if (slot == "displacementMap") { if (params.displacementMap.empty()) params.displacementMap = value; }
        else if (slot == "alphaMap") { if (params.alphaMap.empty()) params.alphaMap = value; }
    };

    for (const auto& [prop, value] : materialInfo) {
        const std::string key = lower(prop);

        if (key == "name") {
            if (const auto* s = std::get_if<std::string>(&value)) params.name = *s;
            continue;
        }

        // An empty value is skipped before the switch upstream.
        if (const auto* s = std::get_if<std::string>(&value); s != nullptr && s->empty()) continue;

        if (key == "kd" || key == "ks" || key == "ke") {
            const auto* color = std::get_if<ColorValue>(&value);
            if (color == nullptr) continue;
            const std::vector<double> components(color->begin(), color->end());
            ::Color converted;
            converted.fromArray(components);
            ColorManagement::colorSpaceToWorking(converted, SRGBColorSpace);
            if (key == "kd") { params.color = converted; params.hasColor = true; }
            else if (key == "ks") { params.specular = converted; params.hasSpecular = true; }
            else { params.emissive = converted; params.hasEmissive = true; }
            continue;
        }

        // The texture options carry their own scale and offset in the line.
        if (key == "map_kd" || key == "map_ks" || key == "map_ke" || key == "norm" ||
            key == "map_bump" || key == "bump" || key == "disp" || key == "map_d") {
            const auto* text = std::get_if<std::string>(&value);
            if (text == nullptr) continue;
            const TextureParams texParams = getTextureParams(*text);
            if (texParams.bumpScale) params.bumpScale = texParams.bumpScale;
            if (texParams.displacementBias) params.displacementBias = texParams.displacementBias;
            if (texParams.displacementScale) params.displacementScale = texParams.displacementScale;

            if (key == "map_kd") setMapForType("map", texParams.url);
            else if (key == "map_ks") setMapForType("specularMap", texParams.url);
            else if (key == "map_ke") setMapForType("emissiveMap", texParams.url);
            else if (key == "norm") setMapForType("normalMap", texParams.url);
            else if (key == "map_bump" || key == "bump") setMapForType("bumpMap", texParams.url);
            else if (key == "disp") setMapForType("displacementMap", texParams.url);
            else {
                setMapForType("alphaMap", texParams.url);
                // An alpha map always forces transparency upstream.
                params.transparent = true;
            }
            continue;
        }

        const auto* text = std::get_if<std::string>(&value);
        if (text == nullptr) continue;
        const double n = std::strtod(text->c_str(), nullptr);

        if (key == "ns") {
            params.shininess = n;
        } else if (key == "d") {
            if (n < 1) {
                params.opacity = n;
                params.transparent = true;
            }
        } else if (key == "tr") {
            double tr = n;
            if (options.invertTrProperty) tr = 1.0 - tr;
            if (tr > 0) {
                params.opacity = 1.0 - tr;
                params.transparent = true;
            }
        }
    }
    return params;
}