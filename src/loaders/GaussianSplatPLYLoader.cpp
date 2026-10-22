#include "loaders/GaussianSplatPLYLoader.h"

#include "loaders/GaussianSplatUtils.h"
#include <cmath>
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

std::map<std::string, std::vector<std::string>> GaussianSplatPLYLoader::propertyMapping(
    int degree) {
    std::map<std::string, std::vector<std::string>> mapping;
    // The four fixed fields have as many names as components.
    mapping["scale"] = {"scale_0", "scale_1", "scale_2"};
    mapping["rotation"] = {"rot_0", "rot_1", "rot_2", "rot_3"};
    mapping["f_dc"] = {"f_dc_0", "f_dc_1", "f_dc_2"};
    mapping["opacity"] = {"opacity"};

    // The rest band is the only part that depends on the degree, and the table is the same
    // one the header scan uses.
    const int components = degree >= 0 && degree < 4 ? kDegreeComponents[degree] : 0;
    std::vector<std::string> rest;
    rest.reserve(static_cast<std::size_t>(components));
    for (int i = 0; i < components; ++i) rest.push_back("f_rest_" + std::to_string(i));
    mapping["f_rest"] = std::move(rest);
    return mapping;
}

GaussianSplatPLYLoader::Converted GaussianSplatPLYLoader::convertGeometry(
    const Attributes& attributes) {
    // The three streams that describe the splat itself have to be present.
    if (attributes.position.empty() || attributes.scale.empty() || attributes.rotation.empty() ||
        attributes.fDc.empty() || attributes.opacity.empty())
        throw std::runtime_error(
            "THREE.GaussianSplatPLYLoader: PLY file requires position, scale, rotation, f_dc and "
            "opacity properties.");

    // The widths are fixed by the format.
    if (attributes.position.size() % 3 != 0 || attributes.scale.size() % 3 != 0 ||
        attributes.rotation.size() % 4 != 0 || attributes.fDc.size() % 3 != 0)
        throw std::runtime_error(
            "THREE.GaussianSplatPLYLoader: Invalid Gaussian splat PLY property itemSize.");

    // Every stream must describe the same number of splats.
    const std::size_t count = attributes.position.size() / 3;
    if (attributes.scale.size() / 3 != count || attributes.rotation.size() / 4 != count ||
        attributes.fDc.size() / 3 != count || attributes.opacity.size() != count)
        throw std::runtime_error(
            "THREE.GaussianSplatPLYLoader: Gaussian splat PLY property counts must match "
            "position.");

    Converted converted;
    converted.count = count;
    converted.centers.resize(count * 3);
    converted.covariances.resize(count * 6);
    converted.colors.resize(count * 4);

    for (std::size_t i = 0; i < count; ++i) {
        // The centre is the position as stored.
        converted.centers[i * 3 + 0] = attributes.position[i * 3 + 0];
        converted.centers[i * 3 + 1] = attributes.position[i * 3 + 1];
        converted.centers[i * 3 + 2] = attributes.position[i * 3 + 2];

        // The scale is a logarithm, so the real size is e to that value.
        const double sx = std::exp(attributes.scale[i * 3 + 0]);
        const double sy = std::exp(attributes.scale[i * 3 + 1]);
        const double sz = std::exp(attributes.scale[i * 3 + 2]);

        // The file stores the quaternion as w x y z, so the first component read is w.
        const double qw = attributes.rotation[i * 4 + 0];
        const double qx = attributes.rotation[i * 4 + 1];
        const double qy = attributes.rotation[i * 4 + 2];
        const double qz = attributes.rotation[i * 4 + 3];

        GaussianSplatUtils::writeCovariance(converted.covariances.data(), i * 6, sx, sy, sz, qx, qy, qz, qw);
        GaussianSplatUtils::writeColorBytesFromSH0(converted.colors.data(), i * 4, attributes.fDc[i * 3 + 0],
                               attributes.fDc[i * 3 + 1], attributes.fDc[i * 3 + 2],
                               GaussianSplatUtils::sigmoid(attributes.opacity[i]));
    }
    return converted;
}
