#include "loaders/LUTCubeLoader.h"

#include "constants.h"

#include <cstdint>
#include <regex>
#include <stdexcept>
#include <vector>

LUTCubeLoader::Result LUTCubeLoader::parse(const std::string& input) const {
    static const std::regex regExpTitle("TITLE +\"([^\"]*)\"");
    static const std::regex regExpSize("LUT_3D_SIZE +(\\d+)");
    static const std::regex regExpDomainMin(
        "DOMAIN_MIN +([\\d.e+-]+) +([\\d.e+-]+) +([\\d.e+-]+)");
    static const std::regex regExpDomainMax(
        "DOMAIN_MAX +([\\d.e+-]+) +([\\d.e+-]+) +([\\d.e+-]+)");
    // Upstream runs this with the m flag, so ^ and $ anchor per line rather than per string.
    static const std::regex regExpDataPoints(
        "^([\\d.e+-]+) +([\\d.e+-]+) +([\\d.e+-]+) *$", std::regex::multiline);

    Result result;
    std::smatch match;

    if (std::regex_search(input, match, regExpTitle)) result.title = match[1].str();

    if (!std::regex_search(input, match, regExpSize))
        throw std::runtime_error("THREE.LUTCubeLoader: Missing LUT_3D_SIZE information");
    result.size = std::stod(match[1].str());

    if (std::regex_search(input, match, regExpDomainMin))
        result.domainMin.set(std::stod(match[1].str()), std::stod(match[2].str()),
                             std::stod(match[3].str()));
    if (std::regex_search(input, match, regExpDomainMax))
        result.domainMax.set(std::stod(match[1].str()), std::stod(match[2].str()),
                             std::stod(match[3].str()));

    if (result.domainMin.x > result.domainMax.x || result.domainMin.y > result.domainMax.y ||
        result.domainMin.z > result.domainMax.z)
        throw std::runtime_error("THREE.LUTCubeLoader: Invalid input domain");

    const std::size_t size = static_cast<std::size_t>(result.size);
    const std::size_t length = size * size * size * 4;
    std::vector<std::uint8_t> bytes(length, 0);

    const bool isByteType = _type == UnsignedByteType;
    const double scale = isByteType ? 255.0 : 1.0;

    std::size_t i = 0;
    for (std::sregex_iterator it(input.begin(), input.end(), regExpDataPoints), end; it != end;
         ++it) {
        if (i + 3 >= bytes.size()) break;
        const auto byteOf = [&](int group) {
            return static_cast<std::uint8_t>(std::stod((*it)[group].str()) * scale);
        };
        bytes[i++] = byteOf(1);
        bytes[i++] = byteOf(2);
        bytes[i++] = byteOf(3);
        // The alpha channel is the scale itself, as upstream writes it.
        bytes[i++] = static_cast<std::uint8_t>(scale);
    }

    auto texture = std::make_shared<Data3DTexture>(Uint8Array(bytes), result.size, result.size,
                                                   result.size);
    texture->type = _type;
    texture->magFilter = LinearFilter;
    texture->minFilter = LinearFilter;
    texture->wrapS = ClampToEdgeWrapping;
    texture->wrapT = ClampToEdgeWrapping;
    texture->wrapR = ClampToEdgeWrapping;
    texture->generateMipmaps = false;
    texture->needsUpdate = true;
    result.texture3D = texture;
    return result;
}
